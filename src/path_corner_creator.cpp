#include <ros/ros.h>
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Float32.h"
#include "std_msgs/Float32MultiArray.h"
#include "ros/console.h"
#include <visualization_msgs/Marker.h>
#include <geometry_msgs/PoseArray.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/Path.h>
#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Core>
#include <eigen_conversions/eigen_msg.h>
#include "tf/tf.h"
#include "tf/transform_listener.h"

//These are globally defined to make it easier to fetch from both callback and main, may the gods of programming smite me and my heresy
visualization_msgs::Marker marker; 
float kinectheight=0.92;

//tf::TransformListener transformer;

std_msgs::Float64MultiArray navigationcorners;
Eigen::MatrixXd eigennavigationcorners(10,14); //Use for simplicity, converted later on. Maximum points that we extract from the path is 10

void Path_Recieved(nav_msgs::Path Path) { //When a path is recieved
	navigationcorners.data.clear();
	int k,x;
	ROS_INFO_STREAM("Path Recieved!");
	ROS_INFO_STREAM("Path length: "<< Path.poses.size());
	for(int i=0; i<Path.poses.size(); i++){
		 //Försökte med tf::Transformer förut, men den ger också undefined reference. Help me Fredrik, you're my only hope
		//tf::Stamped<tf::Pose> newTFpose;
		//tf::Stamped<tf::Pose>& newTFpose2=newTFpose;
		//tf::poseStampedMsgToTF(Path.poses[i],newTFpose2);
		//const tf::Stamped<tf::Pose>& newTFpose1=newTFpose2;
		//transformer.transformPose("/camera_link",newTFpose1,newTFpose2); //Det här fanskapet vill inte fungera, klagade först på att den inte hade ett objekt, därav punktoperatorn och felet ovan
		//tf::poseStampedTFToMsg(newTFpose,Path.poses[i]);

		ROS_INFO_STREAM("Path "<<i<<"recieved: x: " << Path.poses[i].pose.position.x<< ", y: "<< Path.poses[i].pose.position.y<< ", z: "<<Path.poses[i].pose.position.z);
}
	if(Path.poses.size()<10){ //If path is shorter than 10 points, set values used in the for-loop
		x=Path.poses.size();
		eigennavigationcorners.resize(x,14);
	}
	else{ //Ensuring no more than 10 points are being taken from the Path at any length
		x=10;	
		eigennavigationcorners.resize(10,14);
 	}			
	for (int i=0; i<x; i++){ //Looping through all the points in the path
		if(Path.poses.size()<10)
			k=i;//For lengths<10, take all points			
		else
			k=(Path.poses.size()/10*(i+1))-1; //For lengts>10, take 10 points (somewhat) evenly distributet along the path, with the last point always as the last
		//Making 4 corners for each point
		eigennavigationcorners(i,0)=-Path.poses[k].pose.position.y-0.1; //x
		eigennavigationcorners(i,1)=-kinectheight; //y
		eigennavigationcorners(i,2)=Path.poses[k].pose.position.x-0.1; //z According to kinect (Again, kinect and pose.position doesnt get along)

		
		eigennavigationcorners(i,3)=-Path.poses[k].pose.position.y-0.1;
		eigennavigationcorners(i,4)=-kinectheight;
		eigennavigationcorners(i,5)=Path.poses[k].pose.position.x+0.1;

		eigennavigationcorners(i,6)=Path.poses[k].pose.position.y+0.1;
		eigennavigationcorners(i,7)=-kinectheight;
		eigennavigationcorners(i,8)=Path.poses[k].pose.position.x-0.1;

		eigennavigationcorners(i,9)=Path.poses[k].pose.position.y+0.1;
		eigennavigationcorners(i,10)=-kinectheight;
		eigennavigationcorners(i,11)=Path.poses[k].pose.position.x+0.1;
		eigennavigationcorners(i,12)=1000.0;
		eigennavigationcorners(i,13)=1000.0;
	}
	//Make a marker for the last point in the path (used for debugging)
	marker.header.frame_id= "map";
	marker.header.stamp=ros::Time();
	marker.ns="goal";
	marker.id=1;
	marker.lifetime=ros::Duration(5);
	marker.type=visualization_msgs::Marker::CYLINDER;
	marker.action=visualization_msgs::Marker::ADD;
	marker.pose.position.x=Path.poses[Path.poses.size()-1].pose.position.x;
	marker.pose.position.y=Path.poses[Path.poses.size()-1].pose.position.y;
	marker.pose.position.z=Path.poses[Path.poses.size()-1].pose.position.z;
	marker.pose.orientation.x=0.0;
	marker.pose.orientation.y=0.0;
	marker.pose.orientation.z=0.0;
	marker.pose.orientation.w=1.0;
	marker.scale.x=0.3;
	marker.scale.y=0.3;
	marker.scale.z=0.1;
	marker.color.a=1.0;
	marker.color.r=255.0;
	marker.color.g=0.0;
	marker.color.b=0.0;
	tf::matrixEigenToMsg(eigennavigationcorners, navigationcorners); //Convert to ros-compatible publish-format

			
}
void Poses_Recieved(geometry_msgs::PoseArray PeoplePoses) { //If poses recieved (only used during development)
	ROS_INFO_STREAM("Position recieved: x: " << PeoplePoses.poses[0].position.x<< ", y: "<< PeoplePoses.poses[0].position.y<< ", z: "<<PeoplePoses.poses[0].position.z);
			
}

int main(int argc, char** argv){
//Main: Creating node, publishers, subscribers and then forever looping the callbacks
	ros::init(argc, argv, "topictest");
	ros::NodeHandle nh;
	ros::Publisher marker_pub=nh.advertise<visualization_msgs::Marker>("visualization_marker",1000);
	ros::Subscriber People_Poses_sub=nh.subscribe("ground_based_rgbd_people_detector/PeoplePoses",1000, Poses_Recieved);
	ros::Subscriber Path_sub=nh.subscribe("move_base/DWAPlannerROS/global_plan",1000, Path_Recieved);
	ros::Publisher corners_pub=nh.advertise<std_msgs::Float64MultiArray>("path_corner_creator/NavigationCorners",10);
while(ros::ok()){
	ros::spinOnce();
	marker_pub.publish(marker);
	corners_pub.publish(navigationcorners);
}
}

