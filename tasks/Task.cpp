/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <rtt/extras/FileDescriptorActivity.hpp>
#include <math.h>
#include <imu_an_spatial/rs232/rs232.h>
#include <base/logging.h>
#include <base/Time.hpp>
#include <iostream>


#define RADIANS_TO_DEGREES (180.0/M_PI)

using namespace imu_an_spatial;

Task::Task(std::string const& name)
    : TaskBase(name)
{
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
}

Task::~Task()
{
}



/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{

    if(OpenComport(const_cast<char*>(_port.get().c_str()), _baudrate.get())){
        LOG_ERROR_S << "Could not open port." << std::endl;
        return false;
    }
    fd = getFileDescriptor();
    an_decoder_initialise(&an_decoder);

    if (! TaskBase::configureHook())
        return false;
    return true;
}
bool Task::startHook()
{
    RTT::extras::FileDescriptorActivity* activity =
    getActivity<RTT::extras::FileDescriptorActivity>();

    if (activity) activity->watch(fd);

    if (! TaskBase::startHook())
        return false;
    return true;
}
void Task::updateHook()
{
    RTT::extras::FileDescriptorActivity* fd_activity =
            getActivity<RTT::extras::FileDescriptorActivity>();

    base::samples::IMUSensors imu_sample;
    base::samples::RigidBodyState imu_pose;

    if (fd_activity)
    {
        if (fd_activity->hasError())
        {
            //TODO handle fd error
        }
        else if (fd_activity->hasTimeout())
        {
            //TODO handle timeout
        }
        else
        {
            if ((bytes_received = PollComport(an_decoder_pointer(&an_decoder), an_decoder_size(&an_decoder))) > 0)
            {
                /* increment the decode buffer length by the number of bytes received */
                an_decoder_increment(&an_decoder, bytes_received);

                /* decode all the packets in the buffer */
                while ((an_packet = an_packet_decode(&an_decoder)) != NULL)
                {
                    if (an_packet->id == packet_id_system_state) /* system state packet */
                    {
                        /* copy all the binary data into the typedef struct for the packet */
                        /* this allows easy access to all the different values             */
                        if(decode_system_state_packet(&system_state_packet, an_packet) == 0)
                        {
                            imu_pose.time = base::Time::now();
                            imu_pose.sourceFrame = _sourceFrame.get();
                            imu_pose.targetFrame = _targetFrame.get();
                            //TODO lat/long to cartesian, lat/long/height standard deviation, etc.
                            //TODO check for right-handedness of yaw / heading
                            //imu_pose.orientation = base::Vector3d(system_state_packet.orientation[0],system_state_packet.orientation[1],system_state_packet.orientation[2]);
                            imu_pose.velocity = base::Vector3d(system_state_packet.velocity[0],system_state_packet.velocity[1],system_state_packet.velocity[2]);
                            imu_pose.angular_velocity = base::Vector3d(system_state_packet.angular_velocity[0],system_state_packet.angular_velocity[1],system_state_packet.angular_velocity[2]);
                            _imu_pose.write(imu_pose);
                            
                            LOG_INFO("System State Packet:\n");
                            LOG_INFO("\tLatitude = %f, Longitude = %f, Height = %f\n", system_state_packet.latitude * RADIANS_TO_DEGREES, system_state_packet.longitude * RADIANS_TO_DEGREES, system_state_packet.height);
                            LOG_INFO("\tRoll = %f, Pitch = %f, Heading = %f\n", system_state_packet.orientation[0] * RADIANS_TO_DEGREES, system_state_packet.orientation[1] * RADIANS_TO_DEGREES, system_state_packet.orientation[2] * RADIANS_TO_DEGREES);
                        }
                    }
                    else if (an_packet->id == packet_id_raw_sensors) /* raw sensors packet */
                    {
                        /* copy all the binary data into the typedef struct for the packet */
                        /* this allows easy access to all the different values             */
                        if(decode_raw_sensors_packet(&raw_sensors_packet, an_packet) == 0)
                        {
                            imu_sample.time = base::Time::now();
                            imu_sample.acc = base::Vector3d(raw_sensors_packet.accelerometers[0], raw_sensors_packet.accelerometers[1], raw_sensors_packet.accelerometers[2]);
                            imu_sample.gyro = base::Vector3d(raw_sensors_packet.gyroscopes[0], raw_sensors_packet.gyroscopes[1], raw_sensors_packet.gyroscopes[2]);
                            imu_sample.mag = base::Vector3d(raw_sensors_packet.magnetometers[0], raw_sensors_packet.magnetometers[1], raw_sensors_packet.magnetometers[2]);
                            _imu_samples.write(imu_sample);

                            LOG_INFO("Raw Sensors Packet:\n");
                            LOG_INFO("\tAccelerometers X: %f Y: %f Z: %f\n", raw_sensors_packet.accelerometers[0], raw_sensors_packet.accelerometers[1], raw_sensors_packet.accelerometers[2]);
                            LOG_INFO("\tGyroscopes X: %f Y: %f Z: %f\n", raw_sensors_packet.gyroscopes[0] * RADIANS_TO_DEGREES, raw_sensors_packet.gyroscopes[1] * RADIANS_TO_DEGREES, raw_sensors_packet.gyroscopes[2] * RADIANS_TO_DEGREES);
                        }
                    }
                    else
                    {
                        LOG_INFO("Packet ID %u of Length %u\n", an_packet->id, an_packet->length);
                    }

                    /* Ensure that you free the an_packet when your done with it or you will leak memory */
                    an_packet_free(&an_packet);
                }
            }

        }
    }
    
    TaskBase::updateHook();
}
void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    RTT::extras::FileDescriptorActivity* activity =
        getActivity<RTT::extras::FileDescriptorActivity>();
    if (activity)
        activity->clearAllWatches();

    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    TaskBase::cleanupHook();
}