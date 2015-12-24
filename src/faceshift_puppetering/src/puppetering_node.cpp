#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <signal.h>
#include <ros/ros.h>

#include <blender_api_msgs/FSShapekey.h>
#include <blender_api_msgs/FSShapekeys.h>
#include <blender_api_msgs/AnimationMode.h>

#include "fsbinarystream.h"
using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
  std::string ip_num; 
  std::string port_num; 
  ros::init(argc, argv, "faceshift_to_ros");
  ros::NodeHandle nh;

  ros::Publisher pub = nh.advertise<blender_api_msgs::AnimationMode>("/blender_api/set_animation_mode", 30, true);
  ros::Publisher pub_shape = nh.advertise<blender_api_msgs::FSShapekeys>("/blender_api/set_shape_keys", 30);
  try
  {
    blender_api_msgs::FSShapekeys shapekey_pairs;


    fs::fsBinaryStream parserIn, parserOut;
    fs::fsMsgPtr msg;
    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    nh.getParam("IP",ip_num);
    tcp::resolver::query query(ip_num, "33433");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);
    bool firstrun = true;
    size_t len;
    std::vector<std::string> blendshape_names;

    blender_api_msgs::AnimationMode mode;
    mode.value = 1;
    pub.publish(mode);

    for (;;)
    {
      boost::array<char, 128> buf;
      boost::system::error_code error;
      //Now this is an overkill but we need to get the blendshape names in every packet
      if (firstrun)
      {
        fs::fsMsgSendBlendshapeNames bln;
        std::string datatosend;
        parserOut.encode_message(datatosend, bln);
        socket.send(boost::asio::buffer(datatosend));
        firstrun = false;
      }
      len = socket.read_some(boost::asio::buffer(buf), error);
      if (error == boost::asio::error::eof)
      {
        //Now let's do some topic cleanup to handle the errors that arise in the system. 
        ROS_INFO("Socket Terminated");
        //Let's unregister publisher. 
        break; // Connection closed cleanly by peer.
      }
      else if (error)
        throw boost::system::system_error(error); // Some other error.
      parserIn.received(len, buf.data());//As it taked (long int sz, const *data)
      while (msg = parserIn.get_message())
      {

        if (dynamic_cast<fs::fsMsgTrackingState*>(msg.get()))
        {

          fs::fsMsgTrackingState *ts = dynamic_cast<fs::fsMsgTrackingState*>(msg.get());
          const fs::fsTrackingData & data = ts->tracking_data();

          printf ("Time: %f \n", data.m_timestamp);
          printf ("Tracking Results: %s", data.m_trackingSuccessful ? "true" : "false");
          printf ("head translation: %f %f %f\n", data.m_headTranslation.x, data.m_headTranslation.y, data.m_headTranslation.z);
          printf ("head rotation: %f %f %f %f \n", data.m_headRotation.x, data.m_headRotation.y, data.m_headRotation.z, data.m_headRotation.w);
          printf ("Eye Gaze Left Pitch: %f\n", data.m_eyeGazeLeftPitch);
          printf ("Eye Gaze Left Pitch: %f\n", data.m_eyeGazeLeftYaw);
          printf ("Eye Gaze Left Pitch: %f\n", data.m_eyeGazeRightPitch);
          printf ("Eye Gaze Left Pitch: %f\n", data.m_eyeGazeRightYaw);

          std::vector<float> blend_shape = data.m_coeffs;

          int counter = 0;

          if (blendshape_names.size() == blend_shape.size())
          {
            shapekey_pairs.shapekey.clear();
            for (std::vector<float>::iterator i = blend_shape.begin(); i != blend_shape.end(); ++i)
            {
              blender_api_msgs::FSShapekey skey;
              skey.name = blendshape_names[counter];
              skey.value = *i;
              shapekey_pairs.shapekey.push_back(skey);
              printf("Blendshape %s : %f\n", blendshape_names[counter].c_str(),  *i);
              counter++;
            }
            pub_shape.publish(shapekey_pairs);
          }
          else {
            firstrun = true; //Cause the number of blendshapes and the rigging data have now changed in the system.
            printf("There is a mismatch querying for blendshape names. \n");
          }

        }

        if (dynamic_cast<fs::fsMsgBlendshapeNames*>(msg.get()))
        {
          fs::fsMsgBlendshapeNames *bs = dynamic_cast<fs::fsMsgBlendshapeNames*>(msg.get());

          blendshape_names = bs->blendshape_names();

          for (std::vector<std::string>::iterator i = blendshape_names.begin();
               i != blendshape_names.end(); ++i)
          {
            printf("Blendshape name %s\n", (*i).c_str() );
          }

        }


      }
      if (!parserIn.valid()) {
        printf("parser in invalid state\n");
        parserIn.clear();
      }
      //Now include exit statment and other calibrate commands.

    }
  }
  // handle any exceptions that may have been thrown.
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
