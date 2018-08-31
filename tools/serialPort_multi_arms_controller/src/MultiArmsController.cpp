//---------------------------------------------------------------------------------------------------------------------
//  Copyright 2018 Pablo Ramon Soria (a.k.a. Bardo91) pabramsor@gmail.com
//---------------------------------------------------------------------------------------------------------------------
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
//  and associated documentation files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify, merge, publish, distribute,
//  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all copies or substantial
//  portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
//  OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//---------------------------------------------------------------------------------------------------------------------


#include "MultiArmsController.h"

#include <std_msgs/String.h>
#include <fstream>

#include <hecatonquiros/Arm4DoF.h>
#include <hecatonquiros/model_solvers/ModelSolver.h>

#include <chrono>

//---------------------------------------------------------------------------------------------------------------------
bool MultiArmsController::init(int _argc, char** _argv){

	if (_argc != 2) {
        std::cout << "Bad input arguments, please provide only the path of a json config file with the structure detailed in the documentation" << std::endl;
        return false;
    }

    // Create Backends of desired Arms
    std::ifstream rawFile(_argv[1]);
    if (!rawFile.is_open()) {
        std::cout << "Error opening config file" << std::endl;
        return false;
    }

    std::stringstream strStream;
    strStream << rawFile.rdbuf(); //read the file
    std::string json = strStream.str(); //str holds the content of the file

    if(mConfigFile.Parse(json.c_str()).HasParseError()){
        std::cout << "Error parsing json" << std::endl;
        return false;
    }

    ros::NodeHandle nh;

    int nArms = mConfigFile["nArms"].GetInt() + 1;
    mArmsBackend.resize(nArms);
    mEnableBackend.resize(nArms);
    mJointsArmPublisher.resize(nArms);
    mJointIDArmPublisher.resize(nArms);
    mLoadIDArmPublisher.resize(nArms);
    mJointsArmSubscriber.resize(nArms);
    mClawArmService.resize(nArms);
    mJointsArmService.resize(nArms);
    mJointIDArmService.resize(nArms); 
    mLoadIDArmService.resize(nArms);

    for(int i = 1; i < nArms; i++){

        if(mArmsBackend[i].init(json, i)){
            std::cout << "Arm " << std::to_string(i) << " created"<< std::endl;
        }else{
            std::cout << "ERROR creating Arm " << std::to_string(i) << std::endl;
            return false;
        }

        std::string senablebc = "enable_backend"+std::to_string(i);
        const char *cenablebc = senablebc.c_str();
        mEnableBackend[i] = mConfigFile[cenablebc].GetBool();

        //////////////////////////////////////////////////
        // ROS publishers
        mJointsArmPublisher[i] = nh.advertise<sensor_msgs::JointState>("/backendROS/arm_"+std::to_string(i)+"/joints_sub", 1);
        mJointIDArmPublisher[i] = nh.advertise<std_msgs::Int32>("/backendROS/arm_"+std::to_string(i)+"/jointId_sub", 1);
        mLoadIDArmPublisher[i] = nh.advertise<std_msgs::Int32>("/backendROS/arm_"+std::to_string(i)+"/loadId_sub", 1);
        std::cout << "Created ROS Publishers of Arm " << std::to_string(i) << std::endl;

        //////////////////////////////////////////////////
        // ROS subscribers
        mJointsArmSubscriber[i] = nh.subscribe<sensor_msgs::JointState>("/backendROS/arm_"+std::to_string(i)+"/joints", 1, &MultiArmsController::jointsCallback, this);
        std::cout << "Created ROS Subscribers of Arm " << std::to_string(i) << std::endl;

        //////////////////////////////////////////////////
        // ROS services
        mClawArmService[i] = nh.advertiseService("/backendROS/arm_"+std::to_string(i)+"/claw_req", &MultiArmsController::clawService, this);
        mJointsArmService[i] = nh.advertiseService("/backendROS/arm_"+std::to_string(i)+"/joints_req", &MultiArmsController::jointsService, this);
        mJointIDArmService[i] = nh.advertiseService("/backendROS/arm_"+std::to_string(i)+"/jointId_req", &MultiArmsController::jointIDService, this);
        mLoadIDArmService[i] = nh.advertiseService("/backendROS/arm_"+std::to_string(i)+"/loadId_req", &MultiArmsController::loadIDService, this);
        std::cout << "Created ROS Services of Arm " << std::to_string(i) << std::endl;

    }

    return true;
}

// //---------------------------------------------------------------------------------------------------------------------
// bool MultiArmsController::start(){

//     mRun = true;
//     mLoopThread = std::thread(&MultiArmsController::loop, this);   

//     return true;
// }

// //---------------------------------------------------------------------------------------------------------------------
// bool MultiArmsController::stop(){
    
//     mRun = false;
//     if(mLoopThread.joinable()) mLoopThread.join();

//     return true;
// }

// //---------------------------------------------------------------------------------------------------------------------
// void MultiArmsController::loop(){
//     while(ros::ok() && mRun){
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
// }

//---------------------------------------------------------------------------------------------------------------------
void MultiArmsController::jointsCallback(const sensor_msgs::JointStateConstPtr& _msg){

    std::vector<float> jointsArm;
    for(int j = 0; j < _msg->position.size(); j++){
        jointsArm.push_back(_msg->position[j]);
    }
    int id_arm = _msg->effort[0];
    mArmsBackend[id_arm].mBackend->joints(jointsArm, mEnableBackend[id_arm]);

}

//---------------------------------------------------------------------------------------------------------------------
bool MultiArmsController::clawService(hecatonquiros::ReqData::Request &_req, hecatonquiros::ReqData::Response &_res){
    
    if(_req.req){
        mArmsBackend[_req.id].mBackend->claw(_req.data);
    }
    _res.success = true;
    
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool MultiArmsController::jointsService(hecatonquiros::ReqData::Request &_req, hecatonquiros::ReqData::Response &_res){
    
    std::vector<float> joints;
    if(_req.req){
        joints = mArmsBackend[_req.id].mBackend->joints(_req.data);
    }

    sensor_msgs::JointState msg;
    msg.header.stamp = ros::Time::now();
    for(auto&j:joints){
        msg.position.push_back(j);
    }
    mJointsArmPublisher[_req.id].publish(msg);
    _res.success = true;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool MultiArmsController::jointIDService(hecatonquiros::ReqData::Request &_req, hecatonquiros::ReqData::Response &_res){
    
    int pos;
    if(_req.req){
        pos = mArmsBackend[_req.id].mBackend->jointPos(_req.data);
    }

    std_msgs::Int32 msg;
    msg.data = pos;
    mJointIDArmPublisher[_req.id].publish(msg);
    _res.success = true;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool MultiArmsController::loadIDService(hecatonquiros::ReqData::Request &_req, hecatonquiros::ReqData::Response &_res){
    
    int load;
    if(_req.req){
        load = mArmsBackend[_req.id].mBackend->jointLoad(_req.data);
    }

    std_msgs::Int32 msg;
    msg.data = load;
    mLoadIDArmPublisher[_req.id].publish(msg);
    _res.success = true;

    return true;
}
