#include <ros/ros.h>
#include <ros/package.h>
#include <string>
#include <math.h>
#include <sstream>
#include <iostream>

#include <gkd_models/Dynamic.h>
#include <std_msgs/Float64.h>

#include <sensor_msgs/JointState.h>
//inutile mais peut servir pour creer nos propres messages
//#include <Suivi_traj_EK/Commande.h>


using namespace std;

// global variables for subscriber
sensor_msgs::JointState etat;
sensor_msgs::JointState traj;
sensor_msgs::JointState commande;
std_msgs::Float64 commandeq1, commandeq2;
sensor_msgs::JointState jt_state;
gkd_models::Dynamic srv;


void etatCallback(const sensor_msgs::JointStatePtr & msg)
{
    etat = *msg;
}

void trajCallback(const sensor_msgs::JointStatePtr & msg)
{
    traj = *msg;
}


int main (int argc, char** argv)
{
    ros::init(argc, argv, "CTC_controller");
    ros::NodeHandle nh;

    // subscriber Etat (fourni par le modele du grp 1)
    ros::Subscriber etat_sub = nh.subscribe ("/joint_states", 10, etatCallback);

    // subscriber Trajectoire (fourni par le grp 4)
    ros::Subscriber traj_sub = nh.subscribe ("/Trajectoire", 10, trajCallback);

    // publisher effort q1
    ros::Publisher couple1_pub = nh.advertise<std_msgs::Float64>("/hand_effort_controller/command", 10);

    // publisher effort q2
    ros::Publisher couple2_pub = nh.advertise<std_msgs::Float64>("/finger_effort_controller/command", 10);

    // service
    ros::ServiceClient client = nh.serviceClient<gkd_models::Dynamic>("Dynamic");



    float Kp0=1, Kp1=1, Kd0=1, Kd1=1, Ki0=1, Ki1=1, Te=0.01;
    float integrale[2] = {};
    float err[2] = {}, err_p[2] = {};

    ros::Rate rate(1/Te);


    commandeq1.data = 0;
    commandeq2.data = 0;

    etat.position.resize(2);
    etat.velocity.resize(2);

    traj.position.resize(2);
    traj.velocity.resize(2);
    traj.effort.resize(2);
    
    commande.name.resize(2);
    commande.effort.resize(2);
    commande.position.resize(2);
    commande.velocity.resize(2);

    jt_state.name.resize(2);
    jt_state.position.resize(2);
    jt_state.velocity.resize(2);
    jt_state.effort.resize(2);

    while (ros::ok())
    {        
        err[0] = traj.position[0] - etat.position[0];
        err_p[0] = traj.velocity[0] - etat.velocity[0];
        
        integrale[0] += err[0]*Te;
        
        jt_state.effort[0] = Kp0*err[0] + Kd0*err_p[0] + Ki0*integrale[0] + traj.effort[0];
        
        jt_state.name[0] = "q1";


        err[1] = traj.position[1] - etat.position[1];
        err_p[1] = traj.velocity[1] - etat.velocity[1];
        integrale[1] += err[1]*Te;
        jt_state.effort[1] = Kp1*err[1] + Kd1*err_p[1] + Ki1*integrale[1] + traj.effort[1];
        jt_state.name[1] = "q2";

        jt_state.position = etat.position;
        jt_state.velocity = etat.velocity;



        //cout<<"traj position q1: "<<traj.position[0]<<endl;
        //cout<<"traj position q2: "<<traj.position[1]<<endl;
        //cout<<"traj velocity q1: "<<traj.velocity[0]<<endl;
        //cout<<"traj velocity q2: "<<traj.velocity[1]<<endl;
        //cout<<"traj acceleration q1: "<<traj.effort[0]<<endl;
        //cout<<"traj acceleration q2: "<<traj.effort[1]<<endl;
        //cout<<"effort commande q0: "<<jt_state.effort[0]<<endl;

        //cout<<"effort commande q1: "<<jt_state.effort[1]<<endl;

        if (client.exists())
        {
            srv.request.input = jt_state;
            if(client.call(srv))
            {
                commande = srv.response.output;


                cout<<"effort q1= "<<commande.effort[0]<<endl;
                cout<<"effort q2= "<<commande.effort[1]<<endl;

                commandeq1.data = commande.effort[0];
                commandeq2.data = commande.effort[1];

            }
        }
        
        
        // publish setpoint
        couple1_pub.publish(commandeq1);
        couple2_pub.publish(commandeq2);

        ros::spinOnce();
        rate.sleep();
    }

}
