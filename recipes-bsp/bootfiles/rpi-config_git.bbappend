do_deploy:append() {
	echo "dtoverlay=bme280" >> ${DEPLOYDIR}/bcm2711-bootfiles/config.txt
}
