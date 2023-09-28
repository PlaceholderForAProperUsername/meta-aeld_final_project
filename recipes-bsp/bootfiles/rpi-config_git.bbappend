do_deploy:append() {
	echo "dtoverlay=bme280" >> ${DEPLOYDIR}/bcm2835-bootfiles/config.txt
}
