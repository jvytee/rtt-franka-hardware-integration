#ifndef _CONTROL_MODES_H_
#define _CONTROL_MODES_H_

#include <string>
#include <rtt/Port.hpp>
#include <Eigen/Core>
#include <rst-rt/dynamics/JointImpedance.hpp>
#include <rst-rt/dynamics/JointTorques.hpp>
#include <rst-rt/kinematics/JointAngles.hpp>

namespace franka {
    enum ControlModes {Position, Velocity, Torque, Impedance};
    static const std::map < ControlModes, std::string > ControlModeMap = {
        {Position, "JointPositionCtrl"},
        {Velocity, "JointVelocityCtrl"},
        {Torque, "JointTorqueCtrl"},
        {Impedance, "JointImpedanceCtrl"}
    };

    class BaseJointController {
        public:
            virtual RTT::FlowStatus &read() = 0;
            virtual bool connected() = 0;
            virtual Eigen::VectorXf &value() = 0;
            RTT::FlowStatus joint_cmd_fs;
    };

    template < class T > class JointController: public BaseJointController {
        public:
            JointController(const std::string &name, RTT::DataFlowInterface &ports, const ControlModes &control_name, std::function < Eigen::VectorXf & (T &) > conversion_in) : conversion(conversion_in) {
                orocos_port.setName(name + "_" + ControlModeMap.find(control_name)->second);
                orocos_port.doc("Input for " + ControlModeMap.find(control_name)->second + "-cmds from Orocos to Franka.");
                joint_cmd_fs = RTT::NoData;
                ports.addPort(orocos_port);

                joint_cmd_fs = RTT::NoData;
            }

            ~JointController() {}

            RTT::FlowStatus &read() override {
                joint_cmd_fs = orocos_port.read(joint_cmd);
                return joint_cmd_fs;
            }

            RTT::InputPort < T > orocos_port;
            //RTT::FlowStatus joint_cmd_fs;
            T joint_cmd;
            std::function < Eigen::VectorXf & (T &) > conversion;

            bool connected() override {
                return orocos_port.connected();
            }

            Eigen::VectorXf &value() override {
                return conversion(joint_cmd);
            }
    };

    class JointImpedanceController: public BaseJointController {
        public:
        JointImpedanceController(const std::string &name,
                                 RTT::DataFlowInterface &ports,
                                 const ControlModes &control_name,
                                 std::function<Eigen::VectorXf& (rstrt::dynamics::JointImpedance&, rstrt::kinematics::JointAngles&, rstrt::dynamics::JointTorques&)> conversion_in)
            : conversion(conversion_in) {
            impedance_port.setName(name + "_JointImpedanceCtrl");
            impedance_port.doc("Input for JointImpedanceCtrl-cmds from Orocos to Franka.");
            impedance_cmd_fs = RTT::NoData;
            ports.addPort(impedance_port);

            position_port.setName(name + "_JointPosition");
            position_port.doc("Input for JointPosition-cmds from Orocos to Franka while in JointImpedanceCtrl");
            position_cmd_fs = RTT::NoData;
            ports.addPort(position_port);

            torque_port.setName(name + "_JointTorque");
            torque_port.doc("Input for JointTorque-cmds from Orocos to Franka while in JointImpedanceCtrl");
            torque_cmd_fs = RTT::NoData;
            ports.addPort(torque_port);

            joint_cmd_fs = RTT::NoData;
        }

            ~JointImpedanceController() {}

            RTT::FlowStatus &read() override {
                impedance_cmd_fs = impedance_port.read(impedance_cmd);
                position_cmd_fs = position_port.read(position_cmd);
                torque_cmd_fs = torque_port.read(torque_cmd);

                if(impedance_cmd_fs != RTT::NoData) {
                    joint_cmd_fs = impedance_cmd_fs;
                } else if(position_cmd_fs != RTT::NoData) {
                    joint_cmd_fs = position_cmd_fs;
                } else if(torque_cmd_fs != RTT::NoData) {
                    joint_cmd_fs = torque_cmd_fs;
                } else {
                    joint_cmd_fs = RTT::NoData;
                }

                return joint_cmd_fs;
            }

            bool connected() override {
                return impedance_port.connected();
            }

            Eigen::VectorXf &value() override {
                return conversion(impedance_cmd, position_cmd, torque_cmd);
            }

            std::function<Eigen::VectorXf& (rstrt::dynamics::JointImpedance&, rstrt::kinematics::JointAngles&, rstrt::dynamics::JointTorques&)> conversion;

            RTT::InputPort<rstrt::dynamics::JointImpedance> impedance_port;
            RTT::FlowStatus impedance_cmd_fs;
            rstrt::dynamics::JointImpedance impedance_cmd;

            RTT::InputPort<rstrt::kinematics::JointAngles> position_port;
            RTT::FlowStatus position_cmd_fs;
            rstrt::kinematics::JointAngles position_cmd;

            RTT::InputPort<rstrt::dynamics::JointTorques> torque_port;
            RTT::FlowStatus torque_cmd_fs;
            rstrt::dynamics::JointTorques torque_cmd;

    };

    template < class T > class JointFeedback {
        public:
            JointFeedback(std::string name, RTT::DataFlowInterface &ports, std::string feedback_name, std::function < void(T &) > initalization) {
                orocos_port.setName(name + "_" + feedback_name);
                orocos_port.doc("Output for " + feedback_name + " messages from Franka to Orocos.");
                ports.addPort(orocos_port);
                initalization(joint_feedback);
                orocos_port.setDataSample(joint_feedback);
            }

            ~JointFeedback() {}

            T joint_feedback;
            RTT::OutputPort < T > orocos_port;

            void write() {
                orocos_port.write(joint_feedback);
            }

            bool connected() {
                return orocos_port.connected();
            }
    };

    template < class T > class DynamicFeedback {
        public:
            DynamicFeedback(std::string name, RTT::DataFlowInterface &ports, std::string feedback_name, std::function < void(T &) > initalization) {
                orocos_port.setName(name + "_" + feedback_name);
                orocos_port.doc("Output for " + feedback_name + " messages from Franka to Orocos.");
                ports.addPort(orocos_port);
                initalization(dynamicFeedback);
                orocos_port.setDataSample(dynamicFeedback);
            }

            ~DynamicFeedback() {}

            T dynamicFeedback;
            RTT::OutputPort < T > orocos_port;

            void write() {
                orocos_port.write(dynamicFeedback);
            }

            bool connected() {
                return orocos_port.connected();
            }
    };
}
#endif
