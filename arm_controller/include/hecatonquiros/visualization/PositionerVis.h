//---------------------------------------------------------------------------------------------------------------------
//  HECATONQUIROS
//---------------------------------------------------------------------------------------------------------------------
//  Copyright 2018 ViGUS University of Seville
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


#ifndef HECATONQUIROS_ARMCONTROLLER_VISUALIZATION_POSITIONERVIS_H_
#define HECATONQUIROS_ARMCONTROLLER_VISUALIZATION_POSITIONERVIS_H_

#ifdef ENABLE_PCL
#include <Eigen/Eigen>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/PolygonMesh.h>

namespace hecatonquiros{
	class PositionerVis {
	public:
		bool init(pcl::visualization::PCLVisualizer *_viewer, std::string _folderFiles);
		void draw(Eigen::Matrix4f _t0, Eigen::Matrix4f _t1, Eigen::Matrix4f _t2, Eigen::Matrix4f _t3, Eigen::Matrix4f _t4, Eigen::Matrix4f _t5);

	private:
		pcl::visualization::PCLVisualizer *mViewer;
		pcl::PolygonMesh mBarMesh0, mBarMesh1, mBarMesh2, mBarMesh3, mBarMesh4, mBarMesh5;
		pcl::PolygonMesh mFinalMesh;
	};
}

#endif
#endif
