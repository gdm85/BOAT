#include "parameters/parameters.hpp"
#include "utilities/utilities.hpp"
#include "utilities/useful_macros.hpp"
#include "probabilistic/gp.hpp"
#include "optimization/nlopt_interface.hpp"
#include "probabilistic/probabilistic.hpp"
#include "../parameters/parameters.hpp"
#include "../utilities/utilities.hpp"

#include <thread>
#include <chrono>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
using boat::exec;
using boat::RangeParameter;
using boat::AsyncExec;
using std::string;
using std::vector;

string double_to_int_string(double d);

struct QuincyParamsValue {

    // This is called by GP
    vector<double> as_vec() const {
        return {wait_time_factor_, preferred_machine_data_fraction_, preferred_rack_data_fraction_, tor_transfer_cost_,
                core_transfer_cost_};
    }

    string as_string() const {
        string res = "wait_time_factor=" + double_to_int_string(wait_time_factor_) + "\n" +
                     "preferred_machine_data_fraction_=" + std::to_string(preferred_machine_data_fraction_) + "\n" +
                     "preferred_rack_data_fraction_=" + std::to_string(preferred_rack_data_fraction_) + "\n" +
                     "tor_transfer_cost_=" + double_to_int_string(tor_transfer_cost_) + "\n" +
                     "core_transfer_cost_=" + double_to_int_string(core_transfer_cost_) + "\n";
        return res;
    }

    // Quincy cost parameters
    // https://github.com/camsas/firmament/blob/master/src/scheduling/flow/quincy_cost_model.cc
    double wait_time_factor_;
    double preferred_machine_data_fraction_;
    double preferred_rack_data_fraction_;
    double tor_transfer_cost_;
    double core_transfer_cost_;
};


// Contains all Quincy cost model params
struct QuincyParams {

    QuincyParams()
                // Was 200 originally
            : wait_time_factor_(1, 200),
              preferred_machine_data_fraction_(0.05, 0.5),
              preferred_rack_data_fraction_(0.05, 0.5),
              tor_transfer_cost_(2, 1000),
              core_transfer_cost_(2, 2000) {}

    QuincyParamsValue value() {
        QuincyParamsValue res;
        res.wait_time_factor_ = wait_time_factor_.value();
        res.preferred_machine_data_fraction_ = preferred_machine_data_fraction_.value();
        res.preferred_rack_data_fraction_ = preferred_rack_data_fraction_.value();
        res.tor_transfer_cost_ = tor_transfer_cost_.value();
        res.core_transfer_cost_ = core_transfer_cost_.value();

        return res;
    }

    // Wait time factor: high means always try to schedule unscheduled tasks with high priority
    // If too high, you preempt running tasks because unscheduled cost higher than pre-empting
    RangeParameter<int> wait_time_factor_;
    RangeParameter<double> preferred_machine_data_fraction_;
    RangeParameter<double> preferred_rack_data_fraction_;
    // Cost of transferring 1 gb over core or rack switch. Dependency to wait time factor

    RangeParameter<int> tor_transfer_cost_;
    RangeParameter<int> core_transfer_cost_;

};

// All relevant output metrics of a run
struct FirmamentResult {

    string as_string() const {
        string res = "Batch task completion delay = " + std::to_string(median_delay_);
        return res;
    }

    double utility() const {
        return median_delay_;
    }

    double mean_delay_;
    double median_delay_;
    double min_delay_;
    double max_delay_;
    double delay_std_;
    double delay_75_pct_;
    double delay_90_pct_;
    double delay_99_pct_;
};


FirmamentResult
eval_performance(const QuincyParamsValue &params, int iteration);
