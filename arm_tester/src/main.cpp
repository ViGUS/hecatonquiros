//
//
//
//
//



#include <hecatonquiros/Arm4DoF.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <ros/ros.h>
#include <serial/serial.h> 

//--------------------------------------------------------------------------------------------------------------------
void pointsInCircleNU(double _radius, Eigen::Vector3f _center, Eigen::Vector3f n, Eigen::Vector3f u, unsigned _nPoints, std::vector<Eigen::Vector3f> &_poses){
	Eigen::Vector3f nxu = u.cross(n);

	_poses.resize(_nPoints);

	double incT = 2 * M_PI / _nPoints;
	for (unsigned i = 0; i < _nPoints; i++) {
		_poses[i] = _radius*cos(i*incT)*u + _radius*sin(i*incT)*nxu + _center;
	}
}


int main(int _argc, char **_argv) {
	ros::init(_argc, _argv, "arm_tester");

	hecatonquiros::Backend::Config backendConfig1;
	hecatonquiros::Backend::Config backendConfig2;
		
	int mode = 0;
	std::cout << "Press 1 for gazebo backend, 2 for arduino backend and 3 for feetech backend and 4 for no backend: " <<std::endl;
	std::cin >> mode;

	if(mode == 1){	// GAZEBO
		backendConfig1.type = hecatonquiros::Backend::Config::eType::Gazebo; 
		backendConfig1.topic = "aeroarms/left"; 
		backendConfig1.armId =1;
		backendConfig2.type = hecatonquiros::Backend::Config::eType::Gazebo; 
		backendConfig2.topic = "aeroarms/right"; 
		backendConfig2.armId =2;
	}else if(mode == 2){	// ARDUINO
		std::cout << "Write serial port (default /dev/ttyACM0), type 1 for default: ";
		std::string serialPort;
		std::cin >> serialPort;
		if(serialPort == "1"){
			serialPort = "/dev/ttyACM0";
		}
		serial::Serial* arduinoCom = new serial::Serial(serialPort, 115200, serial::Timeout::simpleTimeout(1000));
		if (!arduinoCom->isOpen()) {
			std::cout << "Could not open serial port, exiting" << std::endl;
			return -1;
		}

		backendConfig1.type = hecatonquiros::Backend::Config::eType::Arduino; backendConfig1.sharedSerialPort = arduinoCom; backendConfig1.armId =1;
		backendConfig2.type = hecatonquiros::Backend::Config::eType::Arduino; backendConfig2.sharedSerialPort = arduinoCom; backendConfig2.armId =2;
	}else if(mode == 3){	// feetech
		std::cout << "Write serial port (default/dev/ttyUSB0), type 1 for default: ";
		std::string serialPort;
		std::cin >> serialPort;
		if(serialPort == "1"){
			serialPort = "/dev/ttyUSB0";
		}
		/*
		backendConfig1.valuesMinMax = { {-109.376, 106.492},  
                                    	{-111.490, 107.986},
                                    	{-104.869, 113.775},
                                    	{-111.568, 98.624},
                                    	{-152.400, 144.563},
                                    	{-135.305, 157.744}};
		backendConfig1.jointsOffsets = { 0,	0,	0, 0,	0,	0 };

		backendConfig2.valuesMinMax = { {-112.737, 100.442},  
                                        {-106.202, 110.798},
                                        {-103.401, 116.820},
                                        {-105.499, 110.352},
                                        {-152.098, 136.935},
                                        {-129.274, 161.167}};
		backendConfig2.jointsOffsets = { 0,	0,	0, 0,	0,	0};
		*/

		backendConfig1.configXML = "src/hecatonquiros/arm_controller/config/config_arm2.xml";
		backendConfig2.configXML = "src/hecatonquiros/arm_controller/config/config_arm1.xml";

		backendConfig1.type = hecatonquiros::Backend::Config::eType::Feetech; backendConfig1.port = serialPort; backendConfig1.armId =1;
		backendConfig2.type = hecatonquiros::Backend::Config::eType::Feetech; backendConfig2.port = serialPort; backendConfig2.armId =2;
		
	}else if(mode == 4){	// dummy
		backendConfig1.type = hecatonquiros::Backend::Config::eType::Dummy;
		backendConfig2.type = hecatonquiros::Backend::Config::eType::Dummy;
	}else{
		std::cout << "unrecognized mode, exiting" << std::endl;
        return -1;
	}
	
    hecatonquiros::ModelSolver::Config modelSolverConfig1;
    modelSolverConfig1.type = hecatonquiros::ModelSolver::Config::eType::OpenRave;
    modelSolverConfig1.robotName = "left_arm";
    modelSolverConfig1.manipulatorName = "manipulator";
    modelSolverConfig1.robotFile = _argv[1];
    //modelSolverConfig1.environment = _argv[1];
    modelSolverConfig1.offset = {0.20,0.14,-0.04};
	Eigen::Matrix3f m;
	m = Eigen::AngleAxisf(0*M_PI, Eigen::Vector3f::UnitZ())
		* Eigen::AngleAxisf(1*M_PI,  Eigen::Vector3f::UnitY());
	Eigen::Quaternionf q(m);
    modelSolverConfig1.rotation = {q.w(), q.x(), q.y(), q.z()};
    modelSolverConfig1.visualizer = true;

    hecatonquiros::ModelSolver::Config modelSolverConfig2;
    modelSolverConfig2.type = hecatonquiros::ModelSolver::Config::eType::OpenRave;
    modelSolverConfig2.robotName = "right_arm";
    modelSolverConfig2.manipulatorName = "manipulator";
    modelSolverConfig2.robotFile = _argv[1];
    modelSolverConfig2.offset = {0.2,-0.2,-0.04};
    modelSolverConfig2.rotation = {q.w(), q.x(), q.y(), q.z()};
    modelSolverConfig2.visualizer = true;

	hecatonquiros::Arm4DoF leftArm(modelSolverConfig1, backendConfig1);
	hecatonquiros::Arm4DoF rightArm(modelSolverConfig2, backendConfig2);

	leftArm.home();
	rightArm.home();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	//rightArm.barsSize(0.08,0.15,0.35);
	//leftArm.barsSize(0.08,0.15,0.35);
	
	hecatonquiros::Arm4DoF *armInUse = &rightArm;
	
	bool usingRight = true;
	std::cout << "USING RIGHT ARM" << &armInUse<<std::endl;
	while(true){
		char c;
		while(std::cin >> c) {
			switch(c){
				case 'o':
					std::cout << "open" <<std::endl;
					armInUse->openClaw();
					break;
				case 'c':
					std::cout << "close" <<std::endl;
					armInUse->closeClaw();
					break;
				case 's':
					std::cout << "stop" <<std::endl;
					armInUse->stopClaw();
					break;
				case 'h':
					std::cout << "home" <<std::endl;
					armInUse->home();
					break;
				case 'p':
				{
					float x, y, z;
					bool forceOri;
					std::cout << "position" <<std::endl;
					std::cout << "x: " <<std::endl;
					std::cin >> x;
					std::cout << "y: " <<std::endl;
					std::cin >> y;
					std::cout << "z: " <<std::endl;
					std::cin >> z;
					std::cout << "Force ori: " << std::endl;
					std::cin >> forceOri;
					Eigen::Matrix4f pose = Eigen::Matrix4f::Identity();
					pose(0,3) = x;
					pose(1,3) = y;
					pose(2,3) = z;
					
					hecatonquiros::ModelSolver::IK_TYPE type;
					if(forceOri){
						float dx,dy,dz;
						// std::cout << "dx: " << std::endl;
						// std::cin >> dx;
						// std::cout << "dy: " << std::endl;
						// std::cin >> dy;
						// std::cout << "dz: " << std::endl;
						// std::cin >> dz;
						// Eigen::Vector3f xAxis = {dx, dy, dz};
						// xAxis /=xAxis.norm();	
						std::cout << "dx: " << std::endl;
						std::cin >> dx;
						std::cout << "dy: " << std::endl;
						std::cin >> dy;
						std::cout << "dz: " << std::endl;
						std::cin >> dz;
						Eigen::Vector3f zAxis = {dx, dy, dz};
						zAxis /=zAxis.norm();
						// zAxis -= zAxis.dot(xAxis)*xAxis;

						// Eigen::Vector3f yAxis = zAxis.cross(xAxis);
						// pose.block<3,1>(0,0) = xAxis;
						// pose.block<3,1>(0,1) = yAxis;
						pose.block<3,1>(0,2) = zAxis;
						type = hecatonquiros::ModelSolver::IK_TYPE::IK_6D;

					}else{
						type = hecatonquiros::ModelSolver::IK_TYPE::IK_3D;
					}
					std::vector<float> joints;
					auto start = std::chrono::high_resolution_clock::now();
					if(armInUse->checkIk(pose, joints, type)){
						armInUse->joints(joints, true);
					}else{
						std::cout << "Not found IK" << std::endl;
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
					std::cout << duration.count() << std::endl;
					break;
				}
				case 'r':
				{
					std::vector<float> joints;
					auto pose = armInUse->pose();
					int counter = 0;
					for(;;){
						counter++;
						Eigen::Vector3f incTrans = Eigen::MatrixXf::Random(3,1)*0.005;
						pose.block<3,1>(0,3) += incTrans;
						Eigen::Matrix3f m;
						m = Eigen::AngleAxisf	((double(rand())/RAND_MAX - 0.5)*0.1, 	Eigen::Vector3f::UnitX())
							* Eigen::AngleAxisf	((double(rand())/RAND_MAX - 0.5)*0.1,  	Eigen::Vector3f::UnitY())
							* Eigen::AngleAxisf	((double(rand())/RAND_MAX - 0.5)*0.1, 	Eigen::Vector3f::UnitZ());
						pose.block<3,3>(0,0) = m*pose.block<3,3>(0,0);
						if(armInUse->checkIk(pose, joints, hecatonquiros::ModelSolver::IK_TYPE::IK_6D)){
							armInUse->joints(joints, true);
							std::cout << "Tries: " << counter << std::endl;
							std::cout << incTrans << std::endl;
							std::cout << m << std::endl;	
							std::cout << pose <<std::endl;
							break;
						}else{
							std::cout << "Not found IK" << std::endl;
						}
					}
					break;
				}
				case 'w':
				{
					float dx, dy, dz, distance, step_size, rot_speed;
					int delay;
					std::cout << "Moving aling a line rotating the pose. Pick direction, distance, step size and rotation speed." <<std::endl;
					std::cout << "dx: "; std::cin >> dx;
					std::cout << "dy: "; std::cin >> dy;
					std::cout << "dz: "; std::cin >> dz;
					std::cout << "distance (m): "; std::cin >> distance;
					std::cout << "step_size (m): "; std::cin >> step_size;
					std::cout << "rot_speed (deg): "; std::cin >> rot_speed;
					std::cout << "delay (ms): "; std::cin >> delay;
					if(step_size < 0 || rot_speed == 0){
						std::cout << "Bad step size or rot_speed" <<std::endl;
						continue;
					}
					Eigen::Vector3f dir = {dx, dy, dz};
					dir /=dir.norm();
					int iters = distance/step_size;
					Eigen::Matrix3f rotInc;
					rotInc = Eigen::AngleAxisf(rot_speed/180.0*M_PI, Eigen::Vector3f::UnitX());
					auto pose = armInUse->pose();
					for(unsigned i = 0; i < iters; i++){
						pose.block<3,1>(0,3) +=dir*step_size;
						pose.block<3,3>(0,0) = rotInc*pose.block<3,3>(0,0);
						std::vector<float> joints;
						std::this_thread::sleep_for(std::chrono::milliseconds(delay));
						if(armInUse->checkIk(pose, joints, hecatonquiros::ModelSolver::IK_TYPE::IK_6D)){
							armInUse->joints(joints, true);
						}else{
							if(armInUse->checkIk(pose, joints, hecatonquiros::ModelSolver::IK_TYPE::IK_3D)){
								armInUse->joints(joints, true);
							}else{
								std::cout << "Not found IK" << std::endl;
							}
						}
					}
					break;
				}
				case 'z':
				{
					float circle_radius;
					int delay, nPoints;
					std::cout << "Circle mode." <<std::endl;
					std::cout << "radius: "; std::cin >> circle_radius;
					std::cout << "points: "; std::cin >> circle_radius;
					std::cout << "delay (ms): "; std::cin >> delay;
					auto pose = armInUse->pose();
					std::vector<Eigen::Vector3f> circle_points;
					pointsInCircleNU(circle_radius, pose.block<3,1>(0,3), pose.block<3,1>(0,0), pose.block<3,1>(0,2), nPoints, circle_points);
					for(unsigned i = 0; i < nPoints; i++){
						pose.block<3,1>(0,3) = circle_points[i];
						std::vector<float>joints;
						if(armInUse->checkIk(pose, joints, hecatonquiros::ModelSolver::IK_TYPE::IK_6D)){
							armInUse->joints(joints, true);
						}else{
							if(armInUse->checkIk(pose, joints, hecatonquiros::ModelSolver::IK_TYPE::IK_3D)){
								armInUse->joints(joints, true);
							}else{
								std::cout << "Not found IK" << std::endl;
							}
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(delay));
					}
					break;
				}
				case 'j':
				{	
					std::vector<float> joints = armInUse->joints();
					int n;
					std::cout << "id joint: " <<std::endl;
					std::cin >> n;
					float joint;
					std::cout << "val joint " << n << ": " <<std::endl;
					std::cin >> joint;
					
					joints[n] = joint/180.0*M_PI;
					armInUse->joints(joints, true);
					break;
				}
				case 'J':
				{	
					std::vector<float> joints;
					int n;
					std::cout << "joints" <<std::endl;
					std::cout << "n joints: " <<std::endl;
					std::cin >> n;
					for(unsigned i = 0; i < n; i++){
						float joint;
						std::cout << i << " joint: " <<std::endl;
						std::cin >> joint;
						joints.push_back(joint/180.0*M_PI);
					}
					armInUse->joints(joints, true);
					break;
				}
				case 'i':
				{
					auto pose = armInUse->pose();
					std::cout << pose << std::endl;
					break;
				}
				case 'x':
					if(usingRight){
						usingRight = false;
						armInUse = &leftArm;
						std::cout << "USING LEFT ARM" << std::endl;
					}else{
						usingRight = true;
						armInUse = &rightArm;
						std::cout << "USING RIGHT ARM" << std::endl;
					}
					break;
				case 'l':
				{
					float dx, dy, dz, distance, step_size;
					std::cout << "Moving along a line. Pick direction and distance."<<std::endl;
					std::cout << "dx: "; std::cin >> dx;
					std::cout << "dy: "; std::cin >> dy;
					std::cout << "dz: "; std::cin >> dz;
					std::cout << "distance: "; std::cin >> distance;
					std::cout << "step_size: "; std::cin >> step_size;
					if(step_size < 0){
						std::cout << "Bad step size" <<std::endl;
						continue;
					}
					Eigen::Vector3f dir = {dx, dy, dz};
					dir /=dir.norm();
					int iters = distance/step_size;
					for(unsigned i = 0; i < iters; i++){
						auto pose = armInUse->pose();
						pose.block<3,1>(0,3) +=dir*step_size;
						std::vector<float> joints;
						std::this_thread::sleep_for(std::chrono::milliseconds(30));
						if(armInUse->checkIk(pose, joints, hecatonquiros::ModelSolver::IK_TYPE::IK_3D)){
							armInUse->joints(joints, true);
						}else{
							std::cout << "Not found IK" << std::endl;
						}
					}
					break;
				}
			}	
		}	
	}
}
