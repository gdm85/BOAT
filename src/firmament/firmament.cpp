#include "firmament.hpp"

using boat::ProbEngine;
using boat::SemiParametricModel;
using boat::DAGModel;
using boat::GPParams;
using boat::NLOpt;
using boat::BayesOpt;
using boat::SimpleOpt;
using boat::generator;
using boat::RangeParameter;
using std::uniform_real_distribution;

// First: Naive Bayes opt for Quincy
// Each instance of is a particle for particle filter Monte Carlo
struct UnstructuredQuincyParticle : public SemiParametricModel<UnstructuredQuincyParticle> {

    UnstructuredQuincyParticle() {
        gp_params_.default_noise(0.0);
        gp_params_.mean(uniform_real_distribution<>(0.0, 10.0)(generator));
        gp_params_.stdev(uniform_real_distribution<>(0.0, 200.0)(generator));

        gp_params_.linear_scales({uniform_real_distribution<>(1, 200)(generator), // 0.01 - 2
                                  uniform_real_distribution<>(0.05, 0.5)(generator), // fractions 0.05 - 0.5
                                  uniform_real_distribution<>(0.05, 0.5)(generator),
                                  uniform_real_distribution<>(2, 1000)(generator), // 0.02 - 10 unscaled
                                  uniform_real_distribution<>(2, 2000)( // 0.02 - 20 unscaled
                                          generator) // Core value should be higher because have to go through top of rack anyway
                                 });

        set_params(gp_params_);
    }

    GPParams gp_params_;
};

// Quincy Bayes opt, no structure
struct UnstructuredQuincyModel : public DAGModel<UnstructuredQuincyModel> {

    UnstructuredQuincyModel() {
        particle_engine_.set_num_particles(100000);
    }

    // Model call registers the objective in the DAG model
    void model(const QuincyParams &params) {
        output("objective", particle_engine_, params.wait_time_factor_.value(),
               params.preferred_machine_data_fraction_.value(), params.preferred_rack_data_fraction_.value(),
               params.tor_transfer_cost_.value(), params.core_transfer_cost_.value());
    }

    ProbEngine<UnstructuredQuincyParticle> particle_engine_;
};

void maximize_ei(UnstructuredQuincyModel &quincy_model, QuincyParams &params, double incumbent) {
    std::cout << "Set subopt.. \n";

    NLOpt<> opt(params.wait_time_factor_, params.preferred_machine_data_fraction_,
                params.preferred_rack_data_fraction_, params.tor_transfer_cost_,
                params.core_transfer_cost_);

    auto obj = [&]() {
        double r = quincy_model.expected_improvement("objective", incumbent, params);
        return r;
    };

    opt.set_objective_function(obj);
    opt.set_max_num_iterations(10000);
    opt.set_maximizing();

    std::cout << "Run subopt.. \n";
    opt.run_optimization();
    std::cout << "Finished running subopt.. \n";
}


// Bayes opt for Quincy cost model
void quincy_bayes_opt() {
    UnstructuredQuincyModel quincy_model;
    quincy_model.set_num_particles(100);
    QuincyParams params;

    BayesOpt<std::unordered_map<string, double> > opt;

    // No sampling used, directly optimising expected improvement
    auto subopt = [&]() {
        maximize_ei(quincy_model, params, opt.best_objective());
    };

    // Connect optimisation to firmament execution through eval_performance
    auto objective_function = [&]() {
        std::unordered_map<string, double> res;
        FirmamentResult result = eval_performance(params.value(), opt.iteration_count());

        // Utility is mean completion time of batch tasks
        res["objective"] = result.utility();

        return res;
    };

    // Update model from experiment results
    auto learn = [&](const std::unordered_map<string, double> &r) {
        quincy_model.observe(r, params);
    };

    opt.set_subopt_function(subopt);
    opt.set_objective_function(objective_function);
    opt.set_learning_function(learn);
    opt.set_minimizing();
    opt.set_max_num_iterations(10);

    std::cout << "Start optimisation.. \n";
    opt.run_optimization();
}

// Structured optimisation
// Each instance of is a particle for particle filter Monte Carlo
struct StructuredQuincyParticle : public SemiParametricModel<StructuredQuincyParticle> {

    StructuredQuincyParticle() {
        gp_params_.default_noise(0.0);
        gp_params_.mean(uniform_real_distribution<>(0.0, 10.0)(generator));
        gp_params_.stdev(uniform_real_distribution<>(0.0, 200.0)(generator));
        gp_params_.linear_scales({uniform_real_distribution<>(0.0, 1.0)(generator),
                                  uniform_real_distribution<>(0.0, 1.0)(generator),
                                  uniform_real_distribution<>(0.0, 1.0)(generator),
                                  uniform_real_distribution<>(1.0, 2.0)(generator),
                                  uniform_real_distribution<>(1.0, 5.0)(generator)
                                 });

        set_params(gp_params_);
    }

    double parametric(double wait_time_factor, double preferred_machine_data_fraction, double preferred_rack_data_fraction,
               int tor_transfer_cost, int core_transfer_cost) const {
        return 0.0;
    }

    GPParams gp_params_;
};

// Quincy Bayes opt with structure
struct StructuredQuincyModel : public DAGModel<StructuredQuincyModel> {

    StructuredQuincyModel() {
        particle_engine_.set_num_particles(100000);
    }

    // Model call registers the objective in the DAG model
    void model(const QuincyParams &params) {
        output("objective", particle_engine_, params.wait_time_factor_.value(),
               params.preferred_machine_data_fraction_.value(), params.preferred_rack_data_fraction_.value(),
               params.tor_transfer_cost_.value(), params.core_transfer_cost_.value());
    }

    ProbEngine<StructuredQuincyParticle> particle_engine_;
};

void maximize_structured_ei(StructuredQuincyModel &quincy_model, QuincyParams &params, double incumbent) {
    NLOpt<> opt(params.wait_time_factor_, params.preferred_machine_data_fraction_,
                params.preferred_rack_data_fraction_, params.tor_transfer_cost_,
                params.core_transfer_cost_);

    auto obj = [&]() {
        double r = quincy_model.expected_improvement("objective", incumbent, params);
        return r;
    };

    opt.set_objective_function(obj);
    opt.set_max_num_iterations(10000);
    opt.set_maximizing();
    opt.run_optimization();
}


// For testing static configurations
void run_static_quincy(int iterations) {
    QuincyParams p;

    p.wait_time_factor_.instantiate_in_range(50);
    p.preferred_machine_data_fraction_.instantiate_in_range(0.1);
    p.preferred_rack_data_fraction_.instantiate_in_range(0.1);
    p.tor_transfer_cost_.instantiate_in_range(100);
    p.core_transfer_cost_.instantiate_in_range(200);

    for (int i = 0; i < iterations; ++i) {
        eval_performance(p.value(), i);
    }
}

int main() {
   quincy_bayes_opt();
}
