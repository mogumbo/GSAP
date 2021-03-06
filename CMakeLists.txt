cmake_minimum_required(VERSION 2.8)

# Build option for enabling compiler and linker flags for gathering coverage data
# In command line, set option this way:
# cmake -DCoverage=ON <build folder>
option(Coverage "Coverage" OFF)

# Check command line to see if OpenMP is to be used.
option(UseOpenMP "UseOpenMP" FALSE)

# Build with OpenMP if desired, and package can be found.
if (UseOpenMP)
    message(STATUS "Attempting to find OpenMP package...")
    find_package(OpenMP)
    if (OPENMP_FOUND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D USING_OPENMP")
        message(STATUS "Added OpenMP to buildsystem")
    else()
        message(FATAL_ERROR "Command line asked for OpenMP, but package couldn't be found!")
    endif()
endif()

set(HEADERS
    inc/BenchmarkTimer.h
    inc/CompositeSavePointProvider.h
    inc/ConfigMap.h
    inc/Contracts.h
    inc/DataPoint.h
    inc/DataPoints.h
    inc/DataStore.h
    inc/Datum.h
    inc/DynamicArray.h
    inc/AsyncPrognoser.h
    inc/AsyncPrognoserBuilder.h
    inc/Exceptions.h
    inc/Factory.h
    inc/GaussianVariable.h
    inc/ISavePointProvider.h
    inc/Loading/ConstLoadEstimator.h
    inc/Loading/LoadEstimator.h
    inc/Loading/LoadEstimatorFactory.h
    inc/Loading/MovingAverageLoadEstimator.h
    inc/Matrix.h
    inc/Messages/IMessageProcessor.h
    inc/Messages/IMessagePublisher.h
    inc/Messages/Message.h
    inc/Messages/MessageBus.h
    inc/Messages/MessageClock.h
    inc/Messages/MessageId.h
    inc/Messages/MessageWatcher.h
    inc/Messages/ProgEventMessage.h
    inc/Messages/UDataMessage.h
    inc/Messages/WaypointMessage.h
    inc/ModelBasedAsyncPrognoserBuilder.h
    inc/ModelBasedPrognoser.h
    inc/Models/BatteryModel.h
    inc/Models/SystemModel.h
    inc/Models/SystemModelFactory.h
    inc/Models/PrognosticsModel.h
    inc/Models/PrognosticsModelFactory.h
    inc/Observers/AsyncObserver.h
    inc/Observers/Observer.h
    inc/Observers/ObserverFactory.h
    inc/Observers/ParticleFilter.h
    inc/Observers/UnscentedKalmanFilter.h
    inc/Predictors/AsyncPredictor.h
    inc/Predictors/MonteCarloPredictor.h
    inc/Predictors/Predictor.h
    inc/Predictors/PredictorFactory.h
    inc/PContainer.h
    inc/Point3D.h
    inc/ProgEvent.h
    inc/Prognoser.h
    inc/PrognoserFactory.h
    inc/Singleton.h
    inc/StatisticalTools.h
    inc/StringUtils.h
    inc/Trajectory/AsyncTrajectoryService.h
    inc/Trajectory/ITrajectoryCorrelator.h
    inc/Trajectory/TrajectoryService.h
    inc/ThreadSafeLog.h
    inc/UData.h
    inc/UDataInterfaces.h
)

set(SRCS
    src/ConfigMap.cpp
    src/DataPoint.cpp
    src/DataPoints.cpp
    src/AsyncPrognoserBuilder.cpp
    src/GaussianVariable.cpp
    src/Loading/ConstLoadEstimator.cpp
    src/Loading/GaussianLoadEstimator.cpp
    src/Loading/MovingAverageLoadEstimator.cpp
    src/Matrix.cpp
    src/Messages/EmptyMessage.cpp
    src/Messages/Message.cpp
    src/Messages/MessageBus.cpp
    src/Messages/MessageId.cpp
    src/Messages/WaypointMessage.cpp
    src/ModelBasedAsyncPrognoserBuilder.cpp
    src/ModelBasedPrognoser.cpp
    src/Models/BatteryModel.cpp
    src/Observers/AsyncObserver.cpp
    src/Observers/ParticleFilter.cpp
    src/Observers/UnscentedKalmanFilter.cpp
    src/PContainer.cpp
    src/Predictors/AsyncPredictor.cpp
    src/Predictors/MonteCarloPredictor.cpp
    src/StatisticalTools.cpp
    src/ThreadSafeLog.cpp
    src/Trajectory/AsyncTrajectoryService.cpp
    src/Trajectory/TrajectoryService.cpp
    src/UData.cpp
    src/UDataInterfaces.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/)
add_library(gsap ${HEADERS} ${SRCS})
