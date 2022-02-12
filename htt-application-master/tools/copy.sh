#!/bin/bash

HOST="192.168.2.182"
DIR="app"

FILES="
/source/tools/run.sh
/source/tools/calib.sh
/source/solution/makefile/build
/opt/ti-processor-sdk-linux-am57xx-evm-06.03.00.106/linux-devkit/sysroots/armv7at2hf-neon-linux-gnueabi/lib/liblog4cpp.so.5.0.6
/source/tests/ImageProcessing.Tester/images
/source/calibration
"

scp -r ${FILES} root@${HOST}:${DIR}

# Overwrite the frames path for scheimpflug_transformation_calibration in the setting.json file
custom_setting_file="/source/solution/makefile/build/conf/tmp_settings.json"
cp /source/solution/makefile/build/conf/settings.json ${custom_setting_file}
sed -i 's+../../calibration+./calibration+g' ${custom_setting_file}
scp -r ${custom_setting_file} root@${HOST}:${DIR}/build/conf/settings.json
rm ${custom_setting_file}