detectorID=growth-fy2016g
portNumber=2107
HOMEDIR=/home/pi

cp -r ${HOMEDIR}/git/GROWTH-DAQ/scripts/fy2017_scripts/scripts ${HOMEDIR}
cd ${HOMEDIR}/scripts/stand_alone_system
mv fits_backup.sh_sample fits_backup.sh
mv auto_portforward.sh_sample auto_portforward.sh
sed -i -e "s/growth-fy2017/${detectorID}/g" fits_backup.sh
sed -i -e "s/xxxx/${portNumber}/g" auto_portforward.sh
cd ${HOMEDIR}
cp ${HOMEDIR}/git/GROWTH-DAQ/scripts/fy2017_scripts/growth_config.yaml ${HOMEDIR}/
sed -i -e "s/growth-fy2017/${detectorID}/g" growth_config.yaml
cd ${HOMEDIR}/git/GROWTH-DAQ/daq/configurationFile
cp configuration_growth-fy2017_triggerMode.yaml configuration_${detectorID}.yaml
sed -i -e "s/growth-fy2017/${detectorID}/g" configuration_${detectorID}.yaml
rm ${HOMEDIR}/bootcount.text
rm -rf ${HOMEDIR}/work/growth/data/growth-fy2017z
rm -rf /var/log/growth/*

echo "Modify crontab"
