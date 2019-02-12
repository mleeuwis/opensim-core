/* -------------------------------------------------------------------------- *
 *                            OpenSim:  testIK.cpp                            *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2017 Stanford University and the Authors                *
 * Author(s): Ayman Habib, Ajay Seth, Cassidy Kelly, Peter Loan               *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */


// INCLUDES
#include <OpenSim/Common/Storage.h>
#include "OpenSim/Common/STOFileAdapter.h"
#include "OpenSim/Common/TRCFileAdapter.h"
#include <OpenSim/Common/Reporter.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/OrientationsReference.h>
#include <OpenSim/Simulation/InverseKinematicsSolver.h>
#include <OpenSim/Tools/InverseKinematicsTool.h>
#include <OpenSim/Tools/IKTaskSet.h>
#include <OpenSim/Auxiliary/auxiliaryTestFunctions.h>

using namespace OpenSim;
using namespace std;

void testMarkerWeightAssignments(const std::string& ikSetupFile);
void checkMarkersReferenceConsistencyFromTool(InverseKinematicsTool& ik);

void testInverseKinematicsSolverWithOrientations();
void testInverseKinematicsSolverWithEulerAnglesFromFile();

int main()
{
    int itc = 0;
    SimTK::Array_<std::string> failures;

    try { ++itc;
        testInverseKinematicsSolverWithOrientations(); }
    catch (const std::exception& e) {
        cout << e.what() << endl;
        failures.push_back("testInverseKinematicsSolverWithOrientations");
    }

    try {
        ++itc;
        testInverseKinematicsSolverWithEulerAnglesFromFile();
    }
    catch (const std::exception& e) {
        cout << e.what() << endl;
        failures.push_back("testInverseKinematicsSolverWithEulerAnglesFromFile");
    }

        testMarkerWeightAssignments("subject01_Setup_InverseKinematics.xml");

    Storage  standard("std_subject01_walk1_ik.mot");
    try {
        InverseKinematicsTool ik1("subject01_Setup_InverseKinematics.xml");
        ik1.run();
        Storage result1(ik1.getOutputMotionFileName());
        CHECK_STORAGE_AGAINST_STANDARD(result1, standard, 
            std::vector<double>(24, 0.2), __FILE__, __LINE__, 
            "testInverseKinematicsGait2354 failed");
        cout << "testInverseKinematicsGait2354 passed" << endl;
    }
    catch (const std::exception& e) {
        cout << e.what() << endl;
        failures.push_back("testInverseKinematicsGait2354");
    }

    try {
        InverseKinematicsTool ik2("subject01_Setup_InverseKinematics_NoModel.xml");
        Model mdl("subject01_simbody.osim");
        mdl.initSystem();
        ik2.setModel(mdl);
        ik2.run();
        Storage result2(ik2.getOutputMotionFileName());
        CHECK_STORAGE_AGAINST_STANDARD(result2, standard, 
            std::vector<double>(24, 0.2), __FILE__, __LINE__, 
            "testInverseKinematicsGait2354 GUI workflow failed");
        cout << "testInverseKinematicsGait2354 GUI workflow passed" << endl;
    }
    catch (const std::exception& e) {
        cout << e.what() << endl;
        failures.push_back("testInverseKinematicsGait2354_GUI_workflow");
    }

    try {
        InverseKinematicsTool ik3("constraintTest_setup_ik.xml");
        ik3.run();
        cout << "testInverseKinematicsConstraintTest passed" << endl;
    }
    catch (const std::exception& e) {
        cout << e.what() << endl;
        failures.push_back("testInverseKinematicsConstraintTest");
    }

    if (!failures.empty()) {
        cout << "Done, with " << failures.size() << " failure(s) out of ";
        cout << itc << " test cases." << endl;
        cout << "Failure(s): " << failures << endl;
        return 1;
    }

    cout << "Done. All cases passed." << endl;
    return 0;
}

void testMarkerWeightAssignments(const std::string& ikSetupFile)
{
    // Check the consistency for the IKTaskSet in the IKTool setup
    InverseKinematicsTool ik(ikSetupFile);

    // Get a copy of the IK tasks
    IKTaskSet tasks = ik.getIKTaskSet();

    // assign different weightings so we can verify the assignments
    int nt = tasks.getSize();
    for (int i=0; i < nt; ++i) {
        tasks[i].setWeight(double(i));
    }

    cout << tasks.dump(true) << endl;
    // update tasks used by the IK tool
    ik.getIKTaskSet() = tasks;

    // perform the check
    checkMarkersReferenceConsistencyFromTool(ik);

    // Now reverse the order and reduce the number of tasks
    // so that marker and tasks lists are no longer the same
    IKTaskSet tasks2;
    tasks2.setName("half_markers");

    for (int i = nt-1; i >= 0 ; i -= 2) {
        tasks2.adoptAndAppend(tasks[i].clone());
    }

    cout << tasks2.dump(true) << endl;
    ik.getIKTaskSet() = tasks2;

    // perform the check
    checkMarkersReferenceConsistencyFromTool(ik);

    //Now use more tasks than there are actual markers
    IKMarkerTask* newMarker = new IKMarkerTask();
    newMarker->setName("A");
    newMarker->setWeight(101);
    //insert at the very beginning
    tasks.insert(0, newMarker);
    newMarker = new IKMarkerTask();
    newMarker->setName("B");
    newMarker->setWeight(101);
    // add one in the middle
    tasks.insert(nt / 2, newMarker);
    newMarker = new IKMarkerTask();
    newMarker->setName("C");
    newMarker->setWeight(103);
    // and one more at the end
    tasks.adoptAndAppend(newMarker);
    tasks.setName("Added superfluous tasks");

    cout << tasks.dump(true) << endl;
    // update the tasks of the IK Tool
    ik.getIKTaskSet() = tasks;

    // perform the check: superfluous tasks should be ignored
    checkMarkersReferenceConsistencyFromTool(ik);
}

void checkMarkersReferenceConsistencyFromTool(InverseKinematicsTool& ik)
{
    MarkersReference markersReference;
    SimTK::Array_<CoordinateReference> coordinateReferences;

    ik.populateReferences(markersReference, coordinateReferences);
    const IKTaskSet& tasks = ik.getIKTaskSet();

    // Need a model to get a state, doesn't matter which model.
    Model model;
    SimTK::State& state = model.initSystem();

    SimTK::Array_<string> names = markersReference.getNames();
    SimTK::Array_<double> weights;
    markersReference.getWeights(state, weights);

    for (unsigned int i=0; i < names.size(); ++i) {
        std::cout << names[i] << ": " << weights[i];
        int ix = tasks.getIndex(names[i]);
        if (ix > -1) {
            cout << " in TaskSet: " << tasks[ix].getWeight() << endl;
            SimTK_ASSERT_ALWAYS(weights[i] == tasks[ix].getWeight(),
                "Mismatched weight to marker task");
        }
        else {
            cout << " default: " << markersReference.get_default_weight() << endl;
            SimTK_ASSERT_ALWAYS(
                weights[i] == markersReference.get_default_weight(),
                "Mismatched weight to default weight");
        }
    }
}

void testInverseKinematicsSolverWithOrientations()
{
    Model model("subject01_simbody.osim");
    // visualize for debugging
    //model.setUseVisualizer(true);
    
    SimTK::State& s0 = model.initSystem();

    auto anglesTable = STOFileAdapter::read("std_subject01_walk1_ik.mot");

    size_t nt = anglesTable.getNumRows();
    const auto& coordNames = anglesTable.getColumnLabels();
    // Vector of times is just the independent column of a TimeSeriesTable
    auto times = anglesTable.getIndependentColumn();
    
    int dataRate = int(round(double(nt - 1) / (times[nt-1] - times[0])));
    // get the coordinates of the model
    const auto& coordinates = model.getComponentList<Coordinate>();

    // get bodies as frames that we want to "sense" rotations
    const auto& bodies = model.getComponentList<Body>();

    // Coordinate values in the data file may not correspond to the order
    // of coordinates in the Model. Therefore it is necessary to build a
    // mapping between the data and the model order
    std::vector<int> mapDataToModel;
    // cycle through the coodinates in the model order and store the
    // corresponding column index in the table according to column name
    for (auto& coord : coordinates) {
        int index = -1;
        auto found = std::find(coordNames.begin(), coordNames.end(), coord.getName());
        if(found != coordNames.end())
            index = (int)std::distance(coordNames.begin(), found);
        mapDataToModel.push_back(index);
    }

    cout << "Read in std_subject01_walk1_ik.mot with " << nt << " rows." << endl;
    cout << "Num coordinates in file: " << coordNames.size() << endl;
    cout << "Num of matched coordinates in model: " << mapDataToModel.size() << endl;

    // Feed Orientationation tracking orientations as Rotation matrices
    TimeSeriesTable_<SimTK::Rotation> orientationsData;
    // Also store orientation as Vec3 of XYZ Euler angles
    TimeSeriesTable_<SimTK::Vec3> eulerData;

    std::vector<std::string> bodyLabels;
    for (auto& body : bodies) {
        bodyLabels.push_back(body.getName());
    }
    orientationsData.setColumnLabels(bodyLabels);
    eulerData.setColumnLabels(bodyLabels);

    // DataRate should be a numerical type and NOT a string,
    // but appease the requirement of the TRC FileAdapter for now
    orientationsData.updTableMetaData()
        .setValueForKey("DataRate", std::to_string(dataRate));
    eulerData.updTableMetaData()
        .setValueForKey("DataRate", std::to_string(dataRate));
    eulerData.updTableMetaData()
        .setValueForKey("Units", std::string("Radians"));

    // Writable row for creating table of Rotations 
    SimTK::RowVector_<SimTK::Rotation> rowRots(int(bodyLabels.size()));
    // Writable row for creating table of Euler angles as Vec3 elements
    SimTK::RowVector_<SimTK::Vec3> rowEuler(int(bodyLabels.size()));

    // Apply the read in coordinate values to the model.
    // Then get the rotation of the Bodies in the model and 
    // store them as Rotations and Euler angles in separate tables.
    for (size_t i = 0; i < nt; ++i) {
        const auto& values = anglesTable.getRowAtIndex(i);
        int cnt = 0;
        for (auto& coord : coordinates) {
            if (mapDataToModel[cnt] >= 0) {
                if(coord.getMotionType() == Coordinate::MotionType::Rotational)
                    coord.setValue(s0, 
                        SimTK::convertDegreesToRadians(values(mapDataToModel[cnt++])));
                else
                    coord.setValue(s0, values(mapDataToModel[cnt++]));
            }
        }
        model.realizePosition(s0);
        //model.getVisualizer().show(s0);

        int nb = 0;
        for (auto& body : bodies) {
            const SimTK::Rotation& rot = body.getTransformInGround(s0).R();
            rowRots[nb] = rot;
            rowEuler[nb++] = rot.convertRotationToBodyFixedXYZ();
        }
        orientationsData.appendRow(times[i], rowRots);
        eulerData.appendRow(times[i], rowEuler);
    }

    STOFileAdapter_<SimTK::Vec3>::write(eulerData, "subject1_walk_euler_angles.sto");

    //cout << orientationsData << endl;
    OrientationsReference oRefs(&orientationsData);
    oRefs.set_default_weight(1.0);

    MarkersReference mRefs{};

    SimTK::Array_<CoordinateReference> coordinateRefs;

    // create the solver given the input data
    InverseKinematicsSolver ikSolver(model, mRefs, oRefs, coordinateRefs);
    ikSolver.setAccuracy(1e-4);

    auto timeRange = oRefs.getValidTimeRange();
    cout << "Time range from: " << timeRange[0] << " to " << timeRange[1] << "s." << endl;
    
    s0.updTime() = timeRange[0];
    ikSolver.assemble(s0);

    for (double time : times) {
        s0.updTime() = time;
        ikSolver.track(s0);
        //model.getVisualizer().show(s0);
    }
}

void testInverseKinematicsSolverWithEulerAnglesFromFile()
{
    Model model("subject01_simbody.osim");
    // visualize for debugging
    //model.setUseVisualizer(true);

    // Add a reporter to get IK computed coordinate values out
    TableReporter* ikReporter = new TableReporter();
    ikReporter->setName("ik_reporter");
    auto coordinates = model.getComponentList<Coordinate>();
    // Hookup reporter inputs to the individual coordinate outputs
    for (auto& coord : coordinates) {
        ikReporter->updInput("inputs").connect(
            coord.getOutput("value"), coord.getName());
    }
    model.addComponent(ikReporter);

    SimTK::State& s0 = model.initSystem();

    MarkersReference mRefs{};
    OrientationsReference oRefs("subject1_walk_euler_angles.sto");
    SimTK::Array_<CoordinateReference> coordRefs{};

    // create the solver given the input data
    const double accuracy = 1e-4;
    InverseKinematicsSolver ikSolver(model, mRefs, oRefs, coordRefs);
    ikSolver.setAccuracy(accuracy);

    auto& times = oRefs.getTimes();

    s0.updTime() = times[0];
    ikSolver.assemble(s0);
    //model.getVisualizer().show(s0);

    for (auto time : times) {
        s0.updTime() = time;
        ikSolver.track(s0);
        //model.getVisualizer().show(s0);
        // realize to report to get reporter to pull values from model
        model.realizeReport(s0);
    }

    auto report = ikReporter->getTable();
    STOFileAdapter::write(report, "ik_euler_tracking_results.sto");

    const auto standard = STOFileAdapter::read("std_subject01_walk1_ik.mot");

    ASSERT(report.getNumRows() == standard.getNumRows());
    ASSERT(report.getNumColumns() == standard.getNumColumns());

    auto reportLabels = report.getColumnLabels();
    auto stdLabels = standard.getColumnLabels();

    std::vector<int> mapStdToReport;
    // cycle through the coordinates in the model order and store the
    // corresponding column index in the table according to column name
    for (auto& label : reportLabels) {
        int index = -1;
        auto found = std::find(stdLabels.begin(), stdLabels.end(), label);
        if (found != stdLabels.end())
            index = (int)std::distance(stdLabels.begin(), found);
        mapStdToReport.push_back(index);
        //cout << "Index for '" << label << "' = " << index << endl;
    }

    //Ignore pelvis coordinates 0-5
    for (int i = 6; i < 23; ++i) {
        auto repVec = report.getDependentColumnAtIndex(i);
        auto stdVec = standard.getDependentColumnAtIndex(mapStdToReport[i]);
        auto error = SimTK::Real(SimTK_RTD)*repVec - stdVec;
        cout << "Column '" << reportLabels[i] << "' has RMSE = "
            << sqrt(error.norm()/17) << "degrees" << endl;
        SimTK_ASSERT1_ALWAYS((sqrt(error.norm()/17) < 0.1),
            "Column '%s' FAILED to meet accuracy of 0.1 degree RMS.",
            reportLabels[i].c_str());
    }
}
