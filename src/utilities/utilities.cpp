#include "utilities.hpp"
#include "useful_macros.hpp"
#include <string>
#include <iostream>
#include <cstdio>
#include <memory>
#include <cstdlib>

namespace boat {
    int AsyncExec::next_id_ = 123;
    std::mutex AsyncExec::mtx_;
    std::string python_lib = "/local/scratch/vd241/anaconda2/bin/python";

    std::string get_time(const std::string &format) {
        std::stringstream ss;
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        ss << std::put_time(&tm, format.c_str());
        return ss.str();
    }

    void add_indent(std::string &code, int indent) {
        int index = 0;
        if (code.empty()) {
            return;
        }
        std::string indent_string(indent, ' ');

        // Add indent spaces
        code.insert(0, indent_string);
        index += indent;
        while (true) {
            index = code.find("\n", index);
            if (index == int(std::string::npos) || index == SIZE(code) - 1) {
                break;
            }
            index += 1;
            code.insert(index, indent_string);
            index += indent;
        }

        // Make sure we have a new line at the end
        if (code[code.size() - 1] != '\n') {
            code.append("\n");
        }
    }

    std::string exec_internal(const char *cmd) {
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
        if (!pipe) return "ERROR";
        char buffer[128];
        std::string result = "";
        while (!feof(pipe.get())) {
            if (fgets(buffer, 128, pipe.get()) != NULL) {
                result += buffer;
            }
        }
        return result;
    }

    std::string exec(std::string instruction, bool flush_stderr, bool printing) {
        if (flush_stderr) {
            instruction = instruction + "";
        }
        printing = true;
        if (printing) {
            PR(instruction);
        }
        std::string output = exec_internal(instruction.c_str());
        if (printing) {
            PR(output);
        }
        return output;
    }

    std::string python_exec_cmd(std::string python_command) {
        return python_lib + " " + python_command;
    }

    std::string python_exec(std::string python_command, bool flush_stderr) {
        return exec(python_exec_cmd(python_command), flush_stderr);
    }

    double weighted_average(double left, double right, double proportion_right) {
        return left * (1.0 - proportion_right) + right * proportion_right;
    }

    double average_vec(const std::vector<double> &v) {
        assert(v.size() > 0);
        double r = 0.0;
        for (const auto &e:v) {
            r += e;
        }
        return r / double(v.size());
    }

    double stdev_vec(const std::vector<double> &v) {
        double mean = average_vec(v);
        assert(v.size() > 0);
        double r = 0.0;
        for (const auto &e:v) {
            r += (e - mean) * (e - mean);
        }
        r /= double(v.size());
        return sqrt(r);
    }

} // end namespace