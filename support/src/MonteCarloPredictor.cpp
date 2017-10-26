/**  MonteCarloPredictor - Header
*   @file      MonteCarloPredictor Class
*   @ingroup   GPIC++
*   @ingroup    Predictors
*
*   @brief     MonteCarloPredictor Class - Class defining the MonteCarloPredictor
*
*   @author    Matthew Daigle
*   @version   0.1.0
*
*   @pre       N/A
*
*      Contact: Matthew Daigle (matthew.j.daigle@nasa.gov)
*      Created: March 22, 2016
*
*   @copyright Copyright (c) 2016 United States Government as represented by
*     the Administrator of the National Aeronautics and Space Administration.
*     All Rights Reserved.
*/

#include <random>
#include <string>
#include <vector>

#include "Exceptions.h"
#include "MonteCarloPredictor.h"
#include "Matrix.h"

namespace PCOE {
    // Configuration Keys
    const std::string EVENT_KEY = "Model.event";
    const std::string PREDICTEDOUTPUTS_KEY = "Model.predictedOutputs";
    const std::string PROCESSNOISE_KEY = "Model.processNoise";
    const std::string NUMSAMPLES_KEY = "Predictor.numSamples";
    const std::string HORIZON_KEY = "Predictor.horizon";
    const std::string INPUTUNCERTAINTY_KEY = "Predictor.inputUncertainty";

    // Other string constants
    const std::string MODULE_NAME = "MonteCarloPredictor";

    // ConfigMap-based Constructor
    MonteCarloPredictor::MonteCarloPredictor(GSAPConfigMap & configMap) : Predictor() {
        // Check for required parameters:
        // model = model to be used for simulation
        // numSamples = number of samples used for prediction
        // horizon = prediction horizon (time units)
        // processNoise = list of variance values for process noise, one for each state (zero-mean assumption)
        // event = name of event to predict
        // inputUncertainty = specification of uncertainty associated with inputParameters in pModel->inputEqn
        configMap.checkRequiredParams({ EVENT_KEY, NUMSAMPLES_KEY, HORIZON_KEY, PREDICTEDOUTPUTS_KEY, PROCESSNOISE_KEY, INPUTUNCERTAINTY_KEY });

        // Set configuration parameters
        numSamples = static_cast<unsigned int>(std::stoul(configMap[NUMSAMPLES_KEY][0]));
        horizon = std::stoul(configMap[HORIZON_KEY][0]);
        event = configMap[EVENT_KEY][0];

        // Set up process noise
        std::vector<std::string> processNoiseStrings = configMap[PROCESSNOISE_KEY];
        for (unsigned int i = 0; i < processNoiseStrings.size(); i++) {
            processNoise.push_back(std::stod(processNoiseStrings[i]));
        }

        // Set up input uncertainty
        std::vector<std::string> inputUncertaintyStrings = configMap[INPUTUNCERTAINTY_KEY];
        for (unsigned int i = 0; i < inputUncertaintyStrings.size(); i++) {
            inputUncertainty.push_back(std::stod(inputUncertaintyStrings[i]));
        }

        // Set up predicted outputs
        predictedOutputs = configMap[PREDICTEDOUTPUTS_KEY];

        log.WriteLine(LOG_INFO, MODULE_NAME, "MonteCarloPredictor created");
    }

    // Set model
    void MonteCarloPredictor::setModel(PrognosticsModel * model) {
        pModel = model;

        // Check that process noise was set consistent with this model
        if (processNoise.size() != pModel->getNumStates()) {
            log.WriteLine(LOG_ERROR, MODULE_NAME, "Process noise size does not equal number of model states");
            throw std::range_error("Process noise size does not equal number of model states");
        }

        // Check that there are an even number input uncertainty parameters
        if (inputUncertainty.size()%2 != 0) {
            // TODO(MD): Assumption here is mean/std, but should generalize this
            log.WriteLine(LOG_ERROR, MODULE_NAME, "Input uncertainty needs an even number of variables");
            throw std::range_error("Input uncertainty needs an even number of variables");
        }

        // Check that there are the correct number of predicted outputs
        if (predictedOutputs.size() != pModel->getNumPredictedOutputs()) {
            log.WriteLine(LOG_ERROR, MODULE_NAME, "Number of predicted outputs does not equal number of model's predicted outputs");
            throw std::range_error("Number of predicted outputs does not equal number of model's predicted outputs");
        }
    }

    // Predict function
    void MonteCarloPredictor::predict(const double tP, const std::vector<UData> & state, ProgData & data) {
        // @todo(MD): This is setup for only a single event to predict, need to extend to multiple events

        // Check that model has been set
        if (pModel == NULL) {
            log.WriteLine(LOG_ERROR, MODULE_NAME, "MonteCarloPredictor does not have a model!");
            throw ConfigurationError("MonteCarloPredictor does not have a model!");
        }

        // Create a random number generator
        static std::random_device rDevice;
        static std::mt19937 generator(rDevice());
        
        Matrix xMean(pModel->getNumStates(), 1);
        
        // Assume for now that UData is mean and covariance type, and so we are assuming multivariate normal
        // NOTE: Can check UData uncertainty type to see what it is and how to handle. Perhaps it would be useful to have general code to deal with this, to get samples from it directly? So don't have to check within here.
        // First step is to construct the mean vector and covariance matrix from the UDatas
        Matrix Pxx(pModel->getNumStates(), pModel->getNumStates());
        for (unsigned int xIndex = 0; xIndex < pModel->getNumStates(); xIndex++) {
            xMean[xIndex][0] = state[xIndex][MEAN];
            std::vector<double> covariance = state[xIndex].getVec(COVAR());
            Pxx.row(xIndex, state[xIndex].getVec(COVAR(0)));
        }
        Matrix xRandom(pModel->getNumStates(), 1);
        auto PxxChol = Pxx.chol();
        
        std::vector<double> inputParameters(inputUncertainty.size()/2+1);
        
        std::vector<double> u(pModel->getNumInputs());
        std::vector<double> z(pModel->getNumPredictedOutputs());
        std::vector<double> noise(pModel->getNumStates());
        
        // For each sample
        for (unsigned int sample = 0; sample < numSamples; sample++) {
            // 1. Sample the state
            // Create state vector
            // Now we have mean vector (x) and covariance matrix (Pxx). We can use that to sample a realization of the state.
            // I need to generate a vector of random numbers, size of the state vector
            // Create standard normal distribution
            static std::normal_distribution<> standardDistribution(0, 1);
            for (unsigned int xIndex = 0; xIndex < pModel->getNumStates(); xIndex++) {
                xRandom[xIndex][0] = standardDistribution(generator);
            }
            // Update with mean and covariance
            xRandom = xMean + PxxChol*xRandom;
            std::vector<double> x = static_cast<std::vector<double>>(xRandom.col(0));

            // 2. Sample the input parameters
            // For now, hard-code and assume Gaussian, but these should be specified somehow in the configMap
            // Assuming that for each input parameter, we have specified mean and standard deviation
            // We have a list of pairs (mean,stddev) for each input parameter
            // The order must correspond to the order of the input parameters in the model:
            //   mean_ip1, stddev_ip1, mean_ip2, stddev_ip2, ...
            for (unsigned int ipIndex = 0; ipIndex < pModel->getNumInputParameters(); ipIndex++) {
                // Create distribution for this input parameter
                std::normal_distribution<> inputParameterDistribution(inputUncertainty[2 * ipIndex], inputUncertainty[2 * ipIndex + 1]);
                // Sample a value for it
                inputParameters[ipIndex] = inputParameterDistribution(generator);
            }
            inputParameters[inputUncertainty.size()/2] = 0;

            // 3. Simulate until time limit reached
            double t = tP;
            unsigned int timeIndex = 0;
            data.events[event].timeOfEvent[sample] = INFINITY;
            
            while (t <= tP + horizon) {
                // Implements variable loading, consisting of a sequence of segments with specified magnitude and duration. Each segment starts at the current magnitude and ramps up to the second. When the end is reached, the last value is held.
                
                // inputParameters must contain an odd number of elements, pairs of (magnitude,duration) with a final magnitude (final duration is infinity)
                if (inputParameters.size() < 1 || inputParameters.size() % 2 == 0) {
                    throw std::range_error("Battery::inputEqn - Incorrect number of input parameters");
                }
                
                // Determine where t lies in the given durations, as this specifies what magnitude to use
                // Here, it is assumed that t and the durations are "consistent", ie, if t0 is 1000 then
                // the durations should be specified relative to that. It would be the responsibility of
                // the user of the battery model to take care of this, since the battery model itself,
                // i.e., the C++ object, does not have any state.
                std::vector<double> inputParameters2(inputParameters.size());
                double elapsedTime = 0;
                for (unsigned int i = 0; i < inputParameters.size()-1; i += 2) {
                    // Update time
                    double previousElapsedTime = elapsedTime;
                    elapsedTime += inputParameters[i + 1];
                    // If t hasn't reached elapsedTime yet, this is the portion to use
                    if (t <= elapsedTime) {
                        // Input is interpolated between this value and next in the list
                        double duration = inputParameters[i+1];
                        double deltaT = t-previousElapsedTime;
                        inputParameters2[0] = inputParameters[i] + (inputParameters[i+2]-inputParameters[i])*deltaT/duration;
                    }
                }
                
                // If we get here, we've run out of portions, so just use the last one
                inputParameters2[0] = inputParameters.back();
                
                // Get inputs for time t
                pModel->inputEqn(t, inputParameters2, u);

                // Check threshold at time t and set timeOfEvent if reaching for first time
                // If timeOfEvent is not set to INFINITY that means we already encountered the event,
                // and we don't want to overwrite that.
                auto & theEvent = data.events[event];
                theEvent.occurrenceMatrix[timeIndex][sample] = pModel->thresholdEqn(t, x, u);
                if (theEvent.occurrenceMatrix[timeIndex][sample] && theEvent.timeOfEvent[sample] == INFINITY) {
                    theEvent.timeOfEvent[sample] = t;
                    continue;
                }

                // Write to system trajectory (model variables for which we are interested in predicted values)
                pModel->predictedOutputEqn(t, x, u, z);
                for (unsigned int p = 0; p < pModel->getNumPredictedOutputs(); p++) {
                    data.sysTrajectories[predictedOutputs[p]][timeIndex][sample] = z[p];
                }

                // Sample process noise - for now, assuming independent
                for (unsigned int xIndex = 0; xIndex < pModel->getNumStates(); xIndex++) {
                    static std::normal_distribution<> noiseDistribution(0, sqrt(processNoise[xIndex]));
                    noise[xIndex] = noiseDistribution(generator);
                }

                // Update state for t to t+dt
                pModel->stateEqn(t, x, u, noise);

                // Update time
                t += pModel->getDt();
                timeIndex++;
            }
        }
    }
}
