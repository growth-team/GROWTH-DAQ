detectorID=growth-fy2016a
portNumber=2101
HOMEDIR=/home/pi

cp -r ${HOMEDIR}/git/GROWTH-DAQ/scripts/fy2017_scripts/scripts ./${HOMEDIR}
cd ${HOMEDIR}/scripts/stand_alone_system
mv fits_backup.sh_sample fits_backup.sh
mv auto_portforward.sh/sample auto_portforward.sh
sed -i -e "s/growth-fy2017z/${detectorID}/g" fits_backup.sh
sed -i -e "s/xxxx/${portNumber}/g" auto_portforward.sh
cd ${HOMEDIR}
sed -i -e "s/growth-fy2017z/${detectorID}/g" growth_config.yaml
cd ${HOMEDIR}/git/GROWTH-DAQ/daq/configurationFile
cp configuration_growth-fy2017z.yaml configuration_${detectorID}.yaml
sed -i -e "s/growth-fy2017z/${detectorID}/g" configuration_${detectorID}.yaml
rm ${HOMEDIR}/bootcount.text
rm -rf ${HOMEDIR}/work/growth/data/growth-fy2017z
rm -rf /var/log/*

echo "Modify crontab"
