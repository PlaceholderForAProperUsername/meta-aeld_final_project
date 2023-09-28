do_deploy_append() {
	echo "dtoverlay=bme280" >> ${DEPLOYDIR}/bcm2835-bootfiles/config.txt
}
