#include <ros/ros.h>
#include <sstream>
#include <string>
#include <iostream>
#include "std_msgs/String.h"
#include <std_msgs/Empty.h>
#include <geometry_msgs/Twist.h>
#include <cstdio>
#include <limits>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
using namespace std;
static bool initialized = false;
static struct termios initial_settings;
double start_time;
double test_flight_time = 10.0;
double battery;
double altitude;
double vx;
double vy;
double vz;
bool linebuffered(bool on = true)
{
struct termios settings;
if (!initialized) return false;
if (tcgetattr(STDIN_FILENO, &settings)) return false;
if (on) settings.c_lflag |= ICANON;
else settings.c_lflag &= ~(ICANON);
if (tcsetattr(STDIN_FILENO, TCSANOW, &settings)) return false;
if (on) setlinebuf(stdin);
else setbuf(stdin, NULL);
return true;
}
//---------------------------------------------------------------------------
bool echo(bool on = true)
{
struct termios settings;
if (!initialized) return false;
if (tcgetattr(STDIN_FILENO, &settings)) return false;
if (on) settings.c_lflag |= ECHO;
else settings.c_lflag &= ~(ECHO);
return 0 == tcsetattr(STDIN_FILENO, TCSANOW, &settings);
}
//---------------------------------------------------------------------------
#define INFINITE (-1)
bool iskeypressed(unsigned timeout_ms = 0)
{
if (!initialized) return false;
struct pollfd pls[1];
pls[0].fd = STDIN_FILENO;
pls[0].events = POLLIN | POLLPRI;
return poll(pls, 1, timeout_ms) > 0;
}
//---------------------------------------------------------------------------
class controller
{
public:
controller(){}
void Init(string a, int b , int argc, char** argv)
{
ros::init(argc, argv, "controller");
command = a;
inFlight = b;
}
string key;
int inFlight;
ros::Publisher pubLand, pubTakeoff, pubReset, pubTwist, pubGetNav;
ros::Subscriber nav_sub;
void getKey();
void sendTakeoff(ros::NodeHandle);
void sendLand(ros::NodeHandle);
void sendReset(ros::NodeHandle);
void setMovement(double, double, double);
void resetTwist();
void sendMovement(ros::NodeHandle);
void autoMode(ros::NodeHandle);
void testMove(ros::Rate, ros::NodeHandle);
void gainAltitude(double, ros::NodeHandle);
void nav_callback(const ardrone_autonomy::Navdata&);
void proportional(double, double, double, double);
double getBattery();
double getAltitude();
geometry_msgs::Twist getTwist();
private:
geometry_msgs::Twist twist_msg;
std_msgs::Empty emp_msg;
string command;
};
/*Initializes the controller object
controller(string a, int b, int argc, char** argv)
{
ros::init(argc, argv, "controller");
command = a;
inFlight = b;
}pubTakeoff
*/
void nav_callback(const ardrone_autonomy::Navdata& msg_in)
{
battery = msg_in.batteryPercent;
//cout << "Getting battery life" << endl;
altitude = msg_in.altd*0.001; //mm to m
vx = msg_in.vx*0.001; // From mm/s to m/s
vy = msg_in.vy*0.001;
}
double controller::getBattery()
{
return battery;
}
double controller::getAltitude()
{
return altitude;
}
geometry_msgs::Twist controller::getTwist()
{
return twist_msg;
}
void controller::proportional(double vx_des, double vy_des, double vz_des, double K)
{
twist_msg.linear.x=vx+K*(vx_des-vx); //{-1 to 1}=K*( m/s - m/s)
twist_msg.linear.y=vy+K*(vy_des-vy);
twist_msg.linear.z=vz+K*(vz_des-vz);
}
//Gets input for commands in the menu mode
void controller::getKey()
{
cout << "Enter Command: ";
getline(cin, command);
key = command;
}
//
void controller::gainAltitude(double alt_des, ros::NodeHandle node)
{
controller::proportional(0, 0, 2.5, 0.75);
//controller::setMovement(0, 0, 0.75 * 10);
controller::sendMovement(node);
controller::resetTwist();
controller::sendMovement(node);
}
//Publishes commands for drone to take off
void controller::sendTakeoff(ros::NodeHandle node)
{
if (inFlight == 0)
{
pubTakeoff.publish(emp_msg);
inFlight = 1;
}
}
//Publishes commands for drone to land
void controller::sendLand(ros::NodeHandle node)
{
controller::resetTwist();
controller::sendMovement(node);
pubLand.publish(emp_msg);
inFlight = 0;
}
//Resets mode on drone -- toggles emergency mode
void controller::sendReset(ros::NodeHandle node)
{
pubReset.publish(emp_msg);
}
//Changes Twist message based on parameters being passed
void controller::setMovement(double pitch, double roll, double yaw)
{
// x - forward (1)/ backward (-1) velocity
// y - left (1) / right (-1) velocity
// z - up (1) /down (-1) velocity
twist_msg.linear.x = 0.11*pitch;
twist_msg.linear.y = 0.11*roll;
twist_msg.linear.z = 0.11*yaw;
}
//Resets Twist message values to 0 -- essentailly hovers drone if in flight
void controller::resetTwist()
{
twist_msg.linear.x = 0;
twist_msg.linear.y = 0;
twist_msg.linear.z = 0;
}
//Sends movement commands to drone
void controller::sendMovement(ros::NodeHandle node)
{
pubTwist.publish(twist_msg);
}
//Moves forward for 3 seconds, moves backwards 3 seconds
//Press Enter for emergency land
void controller::testMove(ros::Rate loop_rate, ros::NodeHandle node)
{
if (inFlight == 0)
{
controller::sendTakeoff(node);
inFlight = 1;
}
start_time = (double)ros::Time::now().toSec();
cout << "Started Time: " << start_time << "\n" << endl;
linebuffered(false);
echo(false);
while ((double)ros::Time::now().toSec() < start_time + test_flight_time)
{
if ((double)ros::Time::now().toSec() < start_time + test_flight_time / 2)
{
controller::setMovement(5, 0, 0);
controller::sendMovement(node);
}
else
{
controller::setMovement(-5,0, 0);
controller::sendMovement(node);
}
ros::spinOnce();
loop_rate.sleep();
if (iskeypressed(500) && cin.get() == '\n') break;
}
controller::resetTwist();
controller::sendMovement(node);
controller::sendLand(node);
echo();
linebuffered();
}
void StartMenu()
{
cout << "Keyboard Controls\n";
cout << "t : Takeoff\n";
cout << "l : Land \n";
cout << "r : Reset\n";
cout << "a : Track Mode (TODO)\n";
cout << "s : Test Flight\n";
cout << "q : Quit\n\n";
}
//Sections lined with dotted lines pertain to code getting input from
//keyboard without having to press Enter
//---------------------------------------------------------------------------
bool initialize()
{
if (!initialized)
{
initialized = (bool)isatty(STDIN_FILENO);
if (initialized)
initialized = (0 == tcgetattr(STDIN_FILENO, &initial_settings));
if (initialized)
std::cin.sync_with_stdio();
}
return initialized;
}
//---------------------------------------------------------------------------
void finalize()
{
if (initialized)
tcsetattr(STDIN_FILENO, TCSANOW, &initial_settings);
}
//---------------------------------------------------------------------------

int main(int argc, char** argv)
{
controller controls;
if (!initialize())
{
return 1;
}
// try {
// linebuffered( false );
// echo( false );
// while (true)
// {
// cout << "Zzz..." << flush;
// if (iskeypressed( 500 ) && cin.get() == '\n') break;
// }
// echo();
// linebuffered();
// }
// catch (...) { }
controls.Init("-1", 0, argc, argv);
ros::NodeHandle node;
controls.pubLand = node.advertise<std_msgs::Empty>("/ardrone/land", 1);
controls.pubReset = node.advertise<std_msgs::Empty>("/ardrone/reset", 1);
controls.pubTakeoff = node.advertise<std_msgs::Empty>("/ardrone/takeoff", 1);
controls.pubTwist = node.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
ros::Rate loop_rate(100);
//int quit = 0;
StartMenu();

/*
while (true)
{
controls.getKey();
if (controls.key.compare("t") == 0)
{
controls.sendTakeoff(node);
cout << "Sent Takeoff\n";
cout << "inFlight: " << controls.inFlight << "\n";
}
else if (controls.key.compare("l") == 0)
{
controls.sendLand(node);
cout << "Sent Land\n";
cout << "inFlight: " << controls.inFlight << "\n";
}
else if (controls.key.compare("r") == 0)
{
controls.sendReset(node);
cout << "Sent Reset\n";
}
else if (controls.key.compare("a") == 0)
{
cout << "Starting AutoMode\nPress Enter to quit autoMode\n";
controls.autoMode(node);
}
else if (controls.key.compare("s") == 0)
{
cout << "Starting Test Flight\nPress Enter to quit Test Flight\n";
controls.testMove(loop_rate, node);
}
else if (controls.key.compare("q") == 0)
{
cout<<"Exiting\n";
break;
}
else
cout << "Try again.";
}

*/

finalize();
ROS_INFO("Closing Node");
exit(0);
}

