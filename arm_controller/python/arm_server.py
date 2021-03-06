#---------------------------------------------------------------------------------------------------------------------
#  HECATONQUIROS
#---------------------------------------------------------------------------------------------------------------------
#  Copyright 2018 ViGUS University of Seville
#---------------------------------------------------------------------------------------------------------------------
#  Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
#  and associated documentation files (the "Software"), to deal in the Software without restriction, 
#  including without limitation the rights to use, copy, modify, merge, publish, distribute, 
#  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is 
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all copies or substantial 
#  portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
#  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES 
#  OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#---------------------------------------------------------------------------------------------------------------------

from openravepy import *
import time
import IPython
import numpy as np
import quaternion

import rospy
from hecatonquiros.srv import *
from geometry_msgs.msg  import *
from sensor_msgs.msg    import *
import threading

import math_utils

class ArmServer:
    #------------------------------------------------------------------------------------------------------------------
    ### Initialize robot, ROS interface and multithreads
    ### \param env: initialized enfironment
    ### \param filePath: path to robot file if not loaded before in the enviroment
    ### \param name: custom name of the robot, if loadded before in the environment robot to reference
    ### \param offset: if defined translational offset of the robot in the environment. Numpy array of len 3 {x, y, z}
    ### \param offset: if defined rotation of the robot in the environment. Numpy array of len 4 {w, x, y, z}
    #------------------------------------------------------------------------------------------------------------------
    def __init__(self, env, filePath, name, offset, rotation):
        # Init services and publishers
        self.mServiceSetJoints          = rospy.Service     (name + '/set_joints', SetJoints, self.handlerSetJoints)
        self.mServiceSetPose            = rospy.Service     (name + '/set_pose', SetPose, self.handlerSetPose)
        self.mJointsPublisher           = rospy.Publisher   (name + '/joints', JointState, queue_size=1)
        self.mPosePublisher             = rospy.Publisher   (name + '/pose', PoseStamped, queue_size=1)
        self.mJointsTransformPublisher  = rospy.Publisher   (name + '/joints_transform', PoseArray, queue_size=1)
        
        # Init openrave "things"
        self.mEnvironment = env
        self.mName = name
        with env:
            if not filePath == '' :
                self.mRobot = env.ReadRobotXMLFile(filePath)
                self.mRobot.SetName(name)

                initTransform = np.array(   [   [1,0,0,offset[0]],
                                                [0,1,0,offset[1]],
                                                [0,0,1,offset[2]],
                                                [0,0,0,1]   ])
                initTransform[0:3, 0:3] = quaternion.as_rotation_matrix(np.quaternion(rotation[0], rotation[1], rotation[2], rotation[3]))
                self.mRobot.SetTransform(initTransform)
                env.Add(self.mRobot)
            else:
                for robot in env.GetRobots():
                    if robot.GetName() == name:
                        self.mRobot = robot
                        
            self.mManip = self.mRobot.SetActiveManipulator('manipulator') # set the manipulator to leftarm
            self.mIkmodel3D = databases.inversekinematics.InverseKinematicsModel(self.mRobot, iktype=IkParameterization.Type.Translation3D)
            self.mIkmodel5D = databases.inversekinematics.InverseKinematicsModel(self.mRobot, iktype=IkParameterization.Type.TranslationDirection5D)

            if not self.mIkmodel3D.load():
                self.mIkmodel3D.autogenerate()

            if not self.mIkmodel5D.load():
                self.mIkmodel5D.autogenerate()

        # Init threading
        self.mRun = True
        self.mPosePubThread                 = threading.Thread(target=self.runPosePublisher,             name='PosePublisher')
        self.mJointsPubThread               = threading.Thread(target=self.runJointsPublisher,           name='JointsPublisher')
        self.mJointsTransformsPubThread     = threading.Thread(target=self.runJointsTransformPublisher,  name='JointsTransformPublisher')
        self.mPosePubThread.start()
        self.mJointsPubThread.start()
        self.mJointsTransformsPubThread.start()

        # Extra variables for visualization 
        self.mHandles = []

        # homming
        reqJoints = SetJointsRequest()
        reqJoints.inJoints.position = [0,0,np.pi/2,0,0]
        self.handlerSetJoints(reqJoints)
        

    #------------------------------------------------------------------------------------------------------------------
    ### Stop threads, ROS interface and uninitialize OpenRAVE 
    #------------------------------------------------------------------------------------------------------------------
    def stop(self):
        self.mRun = False
        # sleep a moment and close threads
        self.mPosePubThread.join(500)
        self.mJointsPubThread.join(500)
        self.mJointsTransformsPubThread.join(500)

    #------------------------------------------------------------------------------------------------------------------
    ### Handler of "SetPose" service
    #------------------------------------------------------------------------------------------------------------------
    def handlerSetPose(self, req):
        res = SetPoseResponse()
        if(req.forceOri):
            if not self.mIkmodel5D.load():
                return SetPoseResponse()

            position = np.array([req.inPose.pose.position.x, req.inPose.pose.position.y, req.inPose.pose.position.z])
            qTarget= np.quaternion(   req.inPose.pose.orientation.w, 
                                req.inPose.pose.orientation.x, 
                                req.inPose.pose.orientation.y, 
                                req.inPose.pose.orientation.z)
            rotTarget = quaternion.as_rotation_matrix(qTarget)
            rotMat = quaternion.as_rotation_matrix(qTarget)
            direction = rotMat[0:3,0]   # Z AXIS

            if(req.single):
                print( Ray(position,direction))
                solution = self.mManip.FindIKSolution(
                                        IkParameterization(Ray(position,direction),
                                        IkParameterization.Type.TranslationDirection5D),
                                        IkFilterOptions.CheckEnvCollisions)
                print(solution)
            else:
                solutions = self.mManip.FindIKSolutions(
                                        IkParameterization(Ray(position,direction),
                                        IkParameterization.Type.TranslationDirection5D),
                                        IkFilterOptions.CheckEnvCollisions)
        else:
            if not self.mIkmodel3D.load():
                return SetPoseResponse()

            position = np.array([req.inPose.pose.position.x, req.inPose.pose.position.y, req.inPose.pose.position.z])
            if(req.single):
                solution = self.mManip.FindIKSolution(
                                        IkParameterization(position,
                                        IkParameterization.Type.Translation3D),
                                        IkFilterOptions.CheckEnvCollisions)
            else:
                solutions = self.mManip.FindIKSolutions(
                                        IkParameterization(position,
                                        IkParameterization.Type.Translation3D),
                                        IkFilterOptions.CheckEnvCollisions)

        res = SetPoseResponse()
        
        if(req.single):
            if not solution is None:
                res.outJoints = [JointState()]
                for j in solution:
                    res.outJoints[0].position.append(j) 
        else:
            if not solutions is None:
                for joints in solutions:
                    resJoints = JointState()
                    for j in joints:
                        resJoints.position.append(j)
                    res.outJoints.append(resJoints)

        if(req.set and len(res.outJoints) > 0):
            reqSetJoints = SetJointsRequest()
            reqSetJoints.inJoints = res.outJoints[0]
            self.handlerSetJoints(reqSetJoints)

        return res
        
    #------------------------------------------------------------------------------------------------------------------
    ### Handler of "SetJoints" service
    #------------------------------------------------------------------------------------------------------------------
    def handlerSetJoints(self, req):
        joints = []
        for j in req.inJoints.position:
            joints.append(j)

        self.mRobot.SetDOFValues(joints)
        joints = self.mRobot.GetDOFValues()
        
        res = SetJointsResponse()
        for j in joints:
            res.outJoints.position.append(j) 

        #Visualization of joints transform
        lengthAxis = 0.05
        self.mHandles = []
        for body in self.mRobot.GetLinks():
            T =body.GetTransform()
            self.mHandles.append(self.mEnvironment.drawlinestrip(points=np.array(( T[0:3,3],
                                                                T[0:3,3] + T[0:3,0]*lengthAxis)),
                                            linewidth=3.0,
                                            colors=np.array(((1,0,0)))))

            self.mHandles.append(self.mEnvironment.drawlinestrip(points=np.array(( T[0:3,3],
                                                                T[0:3,3] + T[0:3,1]*lengthAxis)),
                                            linewidth=3.0,
                                            colors=np.array(((0,1,0)))))


            self.mHandles.append(self.mEnvironment.drawlinestrip(points=np.array(( T[0:3,3],
                                                                T[0:3,3] + T[0:3,2]*lengthAxis)),
                                            linewidth=3.0,
                                            colors=np.array(((0,0,1)))))

        return res

    #------------------------------------------------------------------------------------------------------------------
    ### Body pose publisher thread
    #------------------------------------------------------------------------------------------------------------------
    def runPosePublisher(self):
        rate = rospy.Rate(30) # 10hz
        while(self.mRun):
            orPose = np.array([[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]])
            orPose = self.mManip.GetTransform()

            q = quaternion.from_rotation_matrix(orPose[0:3,0:3])
            poseMsg = PoseStamped()
            poseMsg.pose.position.x = orPose[0,3]
            poseMsg.pose.position.y = orPose[1,3]
            poseMsg.pose.position.z = orPose[2,3]
            poseMsg.pose.orientation.w = q.w
            poseMsg.pose.orientation.x = q.x
            poseMsg.pose.orientation.y = q.y
            poseMsg.pose.orientation.z = q.z
            self.mPosePublisher.publish(poseMsg)
            rate.sleep()

    
    #------------------------------------------------------------------------------------------------------------------
    ### Body joint publisher thread
    #------------------------------------------------------------------------------------------------------------------
    def runJointsPublisher(self):
        rate = rospy.Rate(30) # 10hz
        while(self.mRun):
            orJoints = [] 
            orJoints = self.mRobot.GetDOFValues()

            jointsMsg = JointState()
            for j in orJoints:
                jointsMsg.position.append(j)
                
            self.mJointsPublisher.publish(jointsMsg)
            rate.sleep()

    #------------------------------------------------------------------------------------------------------------------
    ### Body joints transform thread
    #------------------------------------------------------------------------------------------------------------------
    def runJointsTransformPublisher(self):
        rate = rospy.Rate(30) # 10hz
        while(self.mRun):
            orLinks = []
            orLinks = self.mRobot.GetLinks()

            jointsTransform = PoseArray()

            for link in orLinks:
                T = link.GetTransform()
                q = quaternion.from_rotation_matrix(T[0:3,0:3])
                pose = Pose()
                pose.position.x = T[0,3]
                pose.position.y = T[1,3]
                pose.position.z = T[2,3]
                pose.orientation.w = q.w
                pose.orientation.x = q.x
                pose.orientation.y = q.y
                pose.orientation.z = q.z
                jointsTransform.poses.append(pose)

            self.mJointsTransformPublisher.publish(jointsTransform)
            rate.sleep()
