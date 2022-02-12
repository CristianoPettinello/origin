#ifndef __NOTIFICATIONS_H__
#define __NOTIFICATIONS_H__

#include <QString>

namespace Nidek {
namespace Solution {
namespace HTT {
namespace UserApplication {
namespace Notifications {

const QString emptyString = "";
const QString startCalibrationInstruction = " \
    <h3>Start Calibration: starts a new calibration</h3> \
    <h3>Resume Calibration: loads the last calibration file</h3> \
";
const QString stepNotImplemented =
    "<h3 style=\"color : red;\">STEP NOT IMPLEMENTED</h3>Press <b>Next</b> button to continue";
const QString gridAcquired = "Grid image acquired, press <b>Next</b> button";
const QString frameAcquired = "Frame image acquired, press <b>Start Bevel Measurement</b> button";
const QString wrongGridAcquired =
    "Grid image acquired not usable, press <b>Back</b> and start again from previous step.";
const QString calibrationFailed = "Calibration failed, press <b>Start Calculation</b> button again.<br>If calibration "
                                  "repeatedly fails, press <b>Back</b> and start again from previous step.";
const QString lineDetectionFailed =
    "Line detection failed, press <b>Start Calculation</b> button again.<br>If calibration "
    "repeatedly fails, press <b>Back</b> and start again from previous step.";

const QString userInstructionsStep1 = " \
    <OL> \
        <LI> Install the HTT on the Calibration Tool (LTT12-8015) by docking it to the tool shaft \
        <LI> Mount the LTT12-8016 on the tool \
        <LI> Switch ON the HTT LED \
        <LI> Rotate the HTT body around the tool shaft until the slit projection reaches, still passing through, the extreme edge (stylus side) of the reference aperture \
        <LI> Tighten the HTT clamp screws \
        <LI> Save & Proceed (→ report) \
        <LI> Remove the LTT12-8016 \
    </OL> ";
const QString userInstructionsStep2 = " \
    <OL> \
        <LI> Mount the chart unit on the calibration tool (LTT12-8017) \
        <LI> Switch ON the Calibration tool LED \
        <LI> Move the HTT sensor in X and Y until it is roughly centered with the chart \
        <LI> Move the imaging lens barrel along Z in order to properly focus the chart and fasten in place \
        <LI> Stop acquisition \
        <LI> Draw a vertical segment on the image over the G3E4 (V) \
        <LI> Check if the V contrast is ≥0.2 \
        <LI> Draw an horizontal segment on the image over the G4E2 (H) \
        <LI> Check if the H contrast is ≥0.2 \
        <LI> Save & Proceed (→ report) \
        <LI> Remove the LTT12-8017 \
    </OL>";
const QString userInstructionsStep3 = " \
    <OL> \
        <LI> Mount the grid unit on the calibration tool (LTT12-8018) \
        <LI> Switch ON the Calibration tool LED \
        <LI> Move the HTT sensor in X and Y until the 4x4 mm grid sides fall within the acceptance area \
        <LI> Fasten the sensor in place \
        <LI> Save & Proceed (→ report) \
        <LI> Remove the LTT12-8018 \
    </OL>";
const QString userInstructionsStep4 = " \
    <OL> \
        <LI> Start the grid calibration \
        <LI> Check if the grid reconstruction errors are within the acceptance ranges (if not, check if any of the parameters has reached a boundary: this will hint to possible HW issues) \
        <LI> Save & Proceed (→ report) \
    </OL>";
const QString userInstructionsStep5 = " \
    <OL> \
        <LI> Check if more than 5mm x 5mm are visible \
        <LI> Save & Proceed (→ report) \
    </OL>";
const QString userInstructionsStep6 = " \
    <OL> \
        <LI> Mount the LTT12-DJ015 on the tool \
        <LI> Switch ON the HTT LED \
        <LI> Move the illumination lens barrel along Z to minimize the slit width on camera (focus the slit) and fasten in place \
        <LI> Check if the projected slit vertically fills the whole field-of-view of the HTT camera \
        <LI> Save & Proceed (→ report) \
        <LI> Remove the LTT12-DJ015 \
    </OL>";
const QString userInstructionsStep7 = " \
    <OL> \
        <LI> Unmount the HTT from the calibration tool and fix it on the optical bench \
        <LI> Properly position the PRD137 so that the HTT projected slit completely fits into the power meter input aperture \
        <LI> Read the power-meter output at the given current setpoints and report it in the table \
        <LI> Verify the Flux vs Current plot against the minimum reference \
        <LI> Save & Proceed (→ report) \
    </OL>";
const QString userInstructionsStep8 = " \
    <OL> \
        <LI> Mount the HTT into the tracer \
        <LI> Mount the LTT12-8014 on the stylus tip \
        <LI> Switch ON the HTT LED \
        <LI> Rotate the HTT body around the stylus shaft until the slit projection reaches, still passing through, the extreme edge (stylus side) of the reference aperture (use the live image to help with this step) \
        <LI> Tighten the HTT clamp screws \
        <LI> Save & Proceed (→ report) \
        <LI> Remove the LTT12-8014 \
    </OL>";
const QString userInstructionsStep9 = " \
    <OL> \
        <LI> Attach bevel M804 to the LTT12-8000 \
        <LI> Load the LTT12-8000 on the tracer \
        <LI> Set the tracer to H=11 \
        <LI> Manually drive the stylus to each bevel position \
        <LI> Change R until the tip is orthogonal to the frame (dN=0) \
        <LI> Input the exact values of H and dN \
        <LI> Adjust LED and camera controls for best bevel image \
        <LI> Stop the camera stream \
        <LI> Start the calibration \
        <LI> Check if the bevel reconstruction errors are within the acceptance ranges \
        <LI> Save & Proceed (→ report) \
    </OL>";
const QString userInstructionsStep10 = " \
    <OL> \
        <LI> For each bevel M###: \
        <UL> \
          <LI> take a snapshot at the following setpoints: dN = -12, -9, -6, -3, 0,…H = +7, +11, +15 \
        </UL> \
        <LI> For each image: \
        <UL> \
          <LI> Run the lens fitting \
          <LI> Verify the output size agaist the actual size and check if error >50μm \
        </UL> \
        <LI> Verify V FoV \
        <LI> Verify H FoV \
        <LI> Verify linearity \
        <LI> Verify resolution \
    </OL>";

const QString step1Objective = "Projected slit centering";
const QString step2Objective = "Resolution check";
const QString step3Objective = "Sensor centering";
const QString step4Objective = "Coarse Calibration of the transformation parameters (HW validation)";
const QString step5Objective = "Check of the imaging FoV";
const QString step6Objective = "Slit focusing and vertical fitting check";
const QString step7Objective = "HTT power check";
const QString step8Objective = "On-tracer projected slit centering";
const QString step9Objective = "Accurate Calibration of the  transformation parameters";
const QString step10Objective = "Complete system characterization";

const QString step1Comment = "Align the projected slit with the slit calibration tool";
const QString step2Comment = "Find the best focusing position of the sensor along with a rough centering. Verify "
                             "that the system satisfy the resolution specifications";
const QString step3Comment = "Align the sensor in x and y with the grid center";
const QString step4Comment =
    "The Scheimpflug parameters for the Back Ray Tracing Algorithm are calculated from the grid and validated";
const QString step5Comment = "Check if the imaging Field of View is large enough";
const QString step6Comment =
    "The slit projection is verified to completely fill the vertical field-of-view of the system";
const QString step7Comment =
    "The power output of the HTT illumination path is measured and validated against a minimum reference value";
const QString step8Comment = "Align the projected slit with the on-tracer alignment jig";
const QString step9Comment = "The Scheimpflug parameters for the Back Ray Tracing Algorithm are calculated from the "
                             "LTT12-8000 M804 bevel and validated";
const QString step10Comment = "Comparison of HTT measurements of the LTT12-8000 bevels with their actual size";

static QString italicString(QString input)
{
    QString output = "<i>" + input + "</i>";
    return output;
}

} // namespace Notifications
} // namespace UserApplication
} // namespace HTT
} // namespace Solution
} // namespace Nidek

#endif // __NOTIFICATIONS_H__
