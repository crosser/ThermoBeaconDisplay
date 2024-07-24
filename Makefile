upload:
	arduino-cli upload -p /dev/ttyACM0 \
		--fqbn esp32:esp32:esp32 \
		--input-dir ./build .

compile:
	arduino-cli compile --fqbn esp32:esp32:esp32 \
		--build-path ./build .
