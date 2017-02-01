#include "firmament.hpp"

// I/O locations
string RESULT_DIR = "/local/scratch/mks40/firmament_data/results/";
string CONFIG_DIR = "/local/scratch/mks40/firmament_data/input_configs/";
string TRACE_DIR = "/local/scratch/mks40/gtrace/traces/google_trace/";
string FIRMAMENT_DIR = "/local/scratch/mks40/firmament/";
string FIRMAMENT_DATA_DIR = "/local/scratch/mks40/firmament_data/";
string OUTPUT_DIR = "/local/scratch/mks40/firmament_data/firmament_output/";

// Static simulation params, can possibly be optimised later
string SIM_RUNTIME = "3600000000";
string RUN_INCREMENTAL = "false";
string FLOWLESSLY_ALGORITHM = "relax";
string NUM_FILES = "10";
// Num files: check time stamps and adjust files based on runtime, 10 for 1 hour

// File names
string config_file = "sim_input.cfg";
string default_config = "default_config.cfg";
string firmament_sim_start_script = "run_firmament_sim.sh";
string output_processing_script = "process_output.py";

string double_to_int_string(double d) { return std::to_string(int(d)); }

std::vector<string> split(const string &s, char delim) {
    std::vector<string> elems;
    std::stringstream ss(s);
    string item;

    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


// Waits for simulator output
FirmamentResult get_result(string file) {
    string full_path = OUTPUT_DIR + file;

    std::cout << "Processing trace.. \n";
    std::string cmd = "/usr/bin/python2.7 " + FIRMAMENT_DATA_DIR + output_processing_script
                      + " --export_path=" + full_path + " --trace_paths=" + OUTPUT_DIR;
    boat::exec(cmd);

    std::cout << "Reading result file.. \n";
    std::ifstream firmament_output(full_path);
    string line;

    FirmamentResult res;
    // Call output processing script and give it name of result file

    while (getline(firmament_output, line)) {

        if (line.length() > 0) {
            vector<string> contents = split(line, ',');

            string::size_type sz;
            res.mean_delay_ = stod(contents[0]);
            res.median_delay_ = stod(contents[1]);
            res.min_delay_ = stod(contents[2]);
            res.max_delay_ = stod(contents[3]);
            res.delay_std_ = stod(contents[4]);
            res.delay_75_pct_ = stod(contents[5]);
            res.delay_90_pct_ = stod(contents[6]);
            res.delay_99_pct_ = stod(contents[7]);

        }
    }
    std::cout << "Finished parsing result file \n";

    return res;
}

// Write out cost model flags to simulator input
// Procedure: read default config from read-only file,
// then write most recent params
void export_quincy_params(const QuincyParamsValue &params) {
    string default_file = CONFIG_DIR + default_config;

    std::ifstream infile(default_file);
    string line;
    vector<string> content;

    // Read in default file, replace defaul values with latest params
    while (getline(infile, line)) {
        string val = line;

        if (line.length() > 0) {
            vector<string> contents = split(line, '=');
            string key = contents[0];

            if (key.compare("--trace_path") == 0) {
                val = "--trace_path=" + TRACE_DIR;  // Google trace
            } else if (key.compare("--runtime") == 0) {
                val = "--runtime=" + SIM_RUNTIME;
            } else if (key.compare("--num_files_to_process") == 0) {
                val = "--num_files_to_process=" + NUM_FILES;
            } else if (key.compare("--run_incremental_scheduler") == 0) {
                val = "--run_incremental_scheduler=" + RUN_INCREMENTAL;
            } else if (key.compare("--flowlessly_algorithm") == 0) {
                val = "--flowlessly_algorithm=" + FLOWLESSLY_ALGORITHM;
            } else if (key.compare("--generated_trace_path") == 0) {
                val = "--generated_trace_path=" + OUTPUT_DIR;
            } else if (key.compare("--log_dir") == 0) {
                val = "--log_dir=" + OUTPUT_DIR;
            } else if (key.compare("--quincy_wait_time_factor") == 0) {
                val = "--quincy_wait_time_factor=" + std::to_string(params.wait_time_factor_);
            } else if (key.compare("--quincy_preferred_machine_data_fraction") == 0) {
                val = "--quincy_preferred_machine_data_fraction=" +
                      std::to_string(params.preferred_machine_data_fraction_);
            } else if (key.compare("--quincy_preferred_rack_data_fraction") == 0) {
                val = "--quincy_preferred_rack_data_fraction=" +
                      std::to_string(params.preferred_rack_data_fraction_);
            } else if (key.compare("--quincy_tor_transfer_cost") == 0) {
                val = "--quincy_tor_transfer_cost=" +
                      double_to_int_string(params.tor_transfer_cost_);
            } else if (key.compare("--quincy_core_transfer_cost") == 0) {
                val = "--quincy_core_transfer_cost=" +
                      double_to_int_string(params.core_transfer_cost_);
            }
        }
        content.push_back(val);
    }

    infile.close();

    // Write out updated params to the file that is given to firmament
    string param_file = CONFIG_DIR + config_file;
    std::ofstream outfile(param_file);

    if (!outfile.is_open()) {
        std::cout << "Error opening outfile file";
    }
    for (int i = 0; i < (int) content.size(); i++) {
        outfile << content[i] << std::endl;
    }

    outfile.close();
}

// Store optimisation results
void store_results(const QuincyParamsValue &params,
                   const FirmamentResult &result) {

    string path = RESULT_DIR + "quincy_opt.txt";

    std::ofstream resultfile;
    resultfile.open(path, std::ios::out | std::ios::app);

    resultfile << std::to_string(params.wait_time_factor_) << ","
               << std::to_string(params.preferred_machine_data_fraction_) << ","
               << std::to_string(params.preferred_rack_data_fraction_) << ","
               << std::to_string(params.tor_transfer_cost_) << ","
               << std::to_string(params.core_transfer_cost_) << ","
               << std::to_string(result.mean_delay_) << ","
               << std::to_string(result.median_delay_) << ","
               << std::to_string(result.min_delay_) << ","
               << std::to_string(result.max_delay_) << ","
               << std::to_string(result.delay_std_) << ","
               << std::to_string(result.delay_75_pct_) << ","
               << std::to_string(result.delay_90_pct_) << ","
               << std::to_string(result.delay_99_pct_) << "\n";

    resultfile.close();
}

// Start firmament simulation
FirmamentResult run_firmament_sim(const string &result_file) {
    boat::AsyncExec::terminate_all("hasu");

    // Clean up old output
    string cleanup_previous_output = "cd " + OUTPUT_DIR + " && rm -rf *";
    boat::exec(cleanup_previous_output);

    // Sim start script has static path to config input location
    string firmament_init = FIRMAMENT_DATA_DIR + firmament_sim_start_script;
    boat::exec(firmament_init);
    std::cout << "Simulation returned.. \n";

    return get_result(result_file);
}

// Main execution function
FirmamentResult
eval_performance(const QuincyParamsValue &params, int iteration) {
    std::cout << "Running firmament simulator \n";

    // Write out params to config file
    export_quincy_params(params);

    string file = std::to_string(iteration) + "_iteration_output.csv";

    FirmamentResult result = run_firmament_sim(file);

    std::this_thread::sleep_for(std::chrono::seconds(5));

    store_results(params, result);

    // Commit main results
    string commit_results =
            "cd " + RESULT_DIR + " && svn add --force * --auto-props --parents --depth infinity -q && "
                    "svn ci -m \"\"";
    boat::exec(commit_results);

    return result;

}

