#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <signal.h>
#include <ros/ros.h>


#include "fsbinarystream.h"
using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
  ros::init(argc, argv, "faceshift_to_ros");
  try
  {
    // the user should specify the server - the 2nd argument
    // if (argc != 3)
    // {
    //   std::cerr << "Usage: client <host> <port>" << std::endl;
    //   return 1;
    // }
    fs::fsBinaryStream parserIn, parserOut;
    fs::fsMsgPtr msg;

    // Any program that uses asio need to have at least one io_service object
    boost::asio::io_service io_service;

    // Convert the server name that was specified as a parameter to the application, to a TCP endpoint.
    // To do this, we use an ip::tcp::resolver object.
    tcp::resolver resolver(io_service);

    // A resolver takes a query object and turns it into a list of endpoints.
    // We construct a query using the name of the server, specified in argv[1],
    // and the name of the service, in this case "daytime".
    tcp::resolver::query query("localhost", "33433");

    // The list of endpoints is returned using an iterator of type ip::tcp::resolver::iterator.
    // A default constructed ip::tcp::resolver::iterator object can be used as an end iterator.
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Now we create and connect the socket.
    // The list of endpoints obtained above may contain both IPv4 and IPv6 endpoints,
    // so we need to try each of them until we find one that works.
    // This keeps the client program independent of a specific IP version.
    // The boost::asio::connect() function does this for us automatically.
    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);
    bool firstrun = true;
    size_t len;
    std::vector<std::string> blendshape_names;
    // The connection is open. All we need to do now is read the response from the daytime service.
    for (;;)
    {
      // We use a boost::array to hold the received data.
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
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.
      parserIn.received(len, buf.data());//As it taked (long int sz, const *data)
      while (msg = parserIn.get_message())
      {
        // printf("The output of the calss is: %d\n", msg.get()->id() );
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

          //Before pulishing it check if they are the same;
          int counter = 0;

          if (blendshape_names.size() == blend_shape.size())
          {
            for (std::vector<float>::iterator i = blend_shape.begin(); i != blend_shape.end(); ++i)
            {
              printf("Blendshape %s : %f\n", blendshape_names[counter].c_str(),  *i);
              counter++;
            }
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
