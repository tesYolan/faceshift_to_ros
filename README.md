# faceshift_to_ros
A wrapper to ouput the faceshift results to a rostopic /blender_api/Blendshapes. 

# To Start

   roslaunch faceshift_puppetering faceshift_puppetering.launch

The above assumes that faceshift is running on localhost with an IP of 33433. To modify the IP configuration

   roslaunch faceshift_puppetering faceshift_puppetering.launch IP:=ip_holding_faceshift

##TODO
The node closes if the connection is terminated; thus a way to note what's happening in this node is needed. 
