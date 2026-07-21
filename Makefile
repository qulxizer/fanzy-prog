qemu:
	export CONFIG_FANZY_WIFI_AP_ENABLED=0
	idf.py qemu --qemu-extra-args="-nic user,model=open_eth,hostfwd=tcp:127.0.0.1:8080-:80" monitor

run:
	idf.py build flash monitor

