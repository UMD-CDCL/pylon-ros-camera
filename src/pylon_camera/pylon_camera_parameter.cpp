// Copyright 2015 <Magazino GmbH>

#include <pylon_camera/pylon_camera_parameter.h>
#include <string>

namespace pylon_camera
{

PylonCameraParameter::PylonCameraParameter() :
        camera_frame_("pylon_camera"),
        device_user_id_(""),
        binning_(1),
        // ##########################
        //  image intensity settings
        // ##########################
        exposure_(10000.0),
        exposure_given_(false),
        gain_(0.5),
        gain_given_(false),
        gamma_(1.0),
        gamma_given_(false),
        brightness_(100),
        brightness_given_(false),
        brightness_continuous_(false),
        exposure_auto_(true),
        gain_auto_(true),
        // #########################
        frame_rate_(5.0),
        mtu_size_(3000),
        shutter_mode_(SM_DEFAULT)
{}

PylonCameraParameter::~PylonCameraParameter()
{}

void PylonCameraParameter::readFromRosParameterServer(const ros::NodeHandle& nh)
{
    // handling of deprecated parameter naming behaviour
    if ( nh.hasParam("desired_framerate") )
    {
        ROS_ERROR_STREAM("Using parameter 'desired_framerate' is deprecated! "
            << "Please rename it to 'frame_rate'");
        nh.getParam("desired_framerate", frame_rate_);
    }
    if ( nh.hasParam("start_exposure") )
    {
        ROS_ERROR_STREAM("Using parameter 'start_exposure' is deprecated! "
            << "Please look at the default.yaml config file in the "
            << "pylon_camera pkg to see how you should use it. The parameter "
            << "name has changed to 'exposure'");
        nh.getParam("start_exposure", exposure_);
    }
    if ( nh.hasParam("target_gain") )
    {
        ROS_ERROR_STREAM("Using parameter 'target_gain' is deprecated! "
            << "Please look at the default.yaml config file in the "
            << "pylon_camera pkg to see how you should use it. The parameter "
            << "name has changed to 'gain'");
        nh.getParam("target_gain", gain_);
    }
    // handling of deprecated parameter naming behaviour


    nh.param<std::string>("camera_frame", camera_frame_, "pylon_camera");
    nh.param<std::string>("device_user_id", device_user_id_, "");
    nh.param<int>("binning", binning_, 1);

    // ##########################
    //  image intensity settings
    // ##########################

    // > 0: Exposure time in microseconds
    exposure_given_ = nh.hasParam("exposure") || nh.hasParam("start_exposure");
    if ( exposure_given_ )
    {
        nh.getParam("exposure", exposure_);
        std::cout << "exposure is given and has value " << exposure_ << std::endl;
    }

    gain_given_ = nh.hasParam("gain") || nh.hasParam("target_gain");
    if ( gain_given_ )
    {
        nh.getParam("gain", gain_);
        std::cout << "gain is given and has value " << gain_ << std::endl;
    }

    gamma_given_ = nh.hasParam("gamma");
    if ( gamma_given_ )
    {
        nh.getParam("gamma", gamma_);
        std::cout << "gamma is given and has value " << gamma_ << std::endl;
    }

    brightness_given_ = nh.hasParam("brightness");
    if ( brightness_given_ )
    {
        nh.getParam("brightness", brightness_);
        std::cout << "brightness is given and has value " << brightness_
            << std::endl;
        if ( gain_given_ && exposure_given_ )
        {
            ROS_ERROR_STREAM("Gain ('gain') and Exposure Time ('exposure') "
                << "are given as startup ros-parameter and hence assumed to be "
                << "fix! The desired brightness (" << brightness_ << ") can't "
                << "be reached! Will ignore the brightness by only "
                << "setting gain and exposure . . .");
            brightness_given_ = false;
        }
        else
        {
            if ( nh.hasParam("brightness_continuous") )
            {
                nh.getParam("brightness_continuous", brightness_continuous_);
                std::cout << "brightness is continuous" << std::endl;
            }
            if ( nh.hasParam("exposure_auto") )
            {
                nh.getParam("exposure_auto", exposure_auto_);
                std::cout << "exposure is set to auto" << std::endl;
            }
            if ( nh.hasParam("gain_auto") )
            {
                nh.getParam("gain_auto", gain_auto_);
                std::cout << "gain is set to auto" << std::endl;
            }
        }
    }

    if ( nh.hasParam("frame_rate") )
    {
        nh.getParam("frame_rate", frame_rate_);
    }
    if ( nh.hasParam("gige/mtu_size") )
    {
        nh.getParam("gige/mtu_size", mtu_size_);
    }

    if ( !device_user_id_.empty() )
    {
        ROS_INFO_STREAM("Trying to open the following camera: "
            << device_user_id_.c_str());
    }
    else
    {
        ROS_INFO_STREAM("No Device User ID set -> Will open the camera device "
                << "found first");
    }

    std::string shutter_param_string;
    nh.param<std::string>("shutter_mode", shutter_param_string, "");
    if (shutter_param_string == "rolling")
    {
        shutter_mode_ = SM_ROLLING;
    }
    else if (shutter_param_string == "global")
    {
        shutter_mode_ = SM_GLOBAL;
    }
    else if (shutter_param_string == "global_reset")
    {
        shutter_mode_ = SM_GLOBAL_RESET_RELEASE;
    }
    else
    {
        shutter_mode_ = SM_DEFAULT;
    }

    validateParameterSet(nh);
    return;
}

void PylonCameraParameter::validateParameterSet(const ros::NodeHandle& nh)
{
    if ( binning_ > 4 || binning_ < 1 )
    {
        ROS_WARN_STREAM("Unsupported binning settings! Binning is "
                << binning_ << ", but valid are only values in this range: "
                << "[1, 2, 3, 4]! Will reset it to default value (1)");
        binning_ = 1;
    }

    if ( exposure_given_ && ( exposure_ <= 0.0 || exposure_ > 1e7 ) )
    {
        ROS_WARN_STREAM("Desired exposure measured in microseconds not in "
                << "valid range! Exposure time = " << exposure_ << ". Will "
                << "reset it to default value!");
        exposure_given_ = false;
    }

    if ( gain_given_ && ( gain_ < 0.0 || gain_ > 1.0 ) )
    {
        ROS_WARN_STREAM("Desired gain (in percent) not in allowed range! "
                << "Gain = " << gain_ << ". Will reset it to default value!");
        gain_given_ = false;
    }

    if ( brightness_given_ && ( brightness_ < 0.0 || brightness_ > 255 ) )
    {
        ROS_WARN_STREAM("Desired brightness not in allowed range [0 - 255]! "
               << "Brightness = " << brightness_ << ". Will reset it to "
               << "default value!");
        brightness_given_ = false;
    }

    if ( frame_rate_ < 0 && frame_rate_ != -1 )
    {
        ROS_WARN_STREAM("Unexpected frame rate (" << frame_rate_ << "). Will "
                << "reset it to default value which is 5 Hz");
        frame_rate_ = 5.0;
        nh.setParam("frame_rate", frame_rate_);
    }
    return;
}

const std::string& PylonCameraParameter::deviceUserID() const
{
    return device_user_id_;
}

std::string PylonCameraParameter::shutterModeString() const
{
    if ( shutter_mode_ == SM_ROLLING )
    {
        return "rolling";
    }
    else if ( shutter_mode_ == SM_GLOBAL )
    {
        return "global";
    }
    else if ( shutter_mode_ == SM_GLOBAL_RESET_RELEASE )
    {
        return "global_reset";
    }
    else
    {
        return "default_shutter_mode";
    }
}

const std::string& PylonCameraParameter::cameraFrame() const
{
    return camera_frame_;
}

void PylonCameraParameter::setFrameRate(const ros::NodeHandle& nh,
                                        const double& frame_rate)
{
    frame_rate_ = frame_rate;
    nh.setParam("frame_rate", frame_rate_);
}

const double& PylonCameraParameter::frameRate() const
{
    return frame_rate_;
}

}  // namespace pylon_camera
