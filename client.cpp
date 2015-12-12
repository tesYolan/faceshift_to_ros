#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include "fsbinarystream.h"
using boost::asio::ip::tcp;

int main(int argc, char* argv[])
{
  try
  {
    // the user should specify the server - the 2nd argument
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>" << std::endl;
      return 1;
    }
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
    tcp::resolver::query query(argv[1], argv[2]);

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

    // The connection is open. All we need to do now is read the response from the daytime service.
    for (;;)
    {
      // We use a boost::array to hold the received data.
      boost::array<char, 128> buf;
      boost::system::error_code error;

      // The boost::asio::buffer() function automatically determines
      // the size of the array to help prevent buffer overruns.
      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      // When the server closes the connection,
      // the ip::tcp::socket::read_some() function will exit with the boost::asio::error::eof error,
      // which is how we know to exit the loop.
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      // std::cout.write(buf.data(), len);
      //Now the program would have exited if it had reached here without data
      parserIn.received(len, buf.data());//As it taked (long int sz, const *data)

      while (msg = parserIn.get_message())
      {
        if (dynamic_cast<fs::fsMsgTrackingState*>(msg.get()))
        {

          fs::fsMsgTrackingState *ts = dynamic_cast<fs::fsMsgTrackingState*>(msg.get());
          const fs::fsTrackingData & data = ts->tracking_data();

          // for (std::vector<std::string>::iterator i = blend.begin(); i != blend.end(); ++i)
          // {
          //   printf("%s\n", (*i).c_str());
          // }

          //Now this is enough for testing but the name of the blendshapes must be found in the system.
          // std::vector<std::string> blender_shapes = bn->blendshape_names();


          printf ("head translation: %f %f %f\n", data.m_headTranslation.x, data.m_headTranslation.y, data.m_headTranslation.z);
          printf ("head rotation: %f %f %f %f \n", data.m_headRotation.x, data.m_headRotation.y, data.m_headRotation.z, data.m_headRotation.w);
          printf ("Eye Gaze Left Pitch: %f\n", data.m_eyeGazeLeftPitch);
          printf ("Eye Gaze Left Pitch: %f\n", data.m_eyeGazeLeftYaw);
          printf ("Eye Gaze Left Pitch: %f\n", data.m_eyeGazeRightPitch);
          printf ("Eye Gaze Left Pitch: %f\n", data.m_eyeGazeRightYaw);
          int counter = 0;
          std::vector<float> blend_shape = data.m_coeffs;
          for (std::vector<float>::iterator i = blend_shape.begin(); i != blend_shape.end(); ++i)
          {
            printf("Blendshape %d %f\n", counter, *i);
            counter++;
          }
        }
        if (dynamic_cast<fs::fsMsgBlendshapeNames*>(msg.get()))
        {
          fs::fsMsgBlendshapeNames *bn = dynamic_cast<fs::fsMsgBlendshapeNames*>(msg.get());

          std::vector<std::string> blend = bn->blendshape_names();
          int counter = 0;
          for (std::vector<std::string>::iterator i = blend.begin(); i != blend.end(); ++i)
          {
            printf("Blendshape %d %s\n", counter, (*i).c_str());
            counter++;
          }


        }
        else{
          //Not getting data
          printf("Blendshapes not found\n" );
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
