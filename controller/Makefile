LOG_DIR = /var/log/growth
GOD_CONFIG_FILE = /etc/god/growth.god.conf
INIT_SCRIPT_FILE = /etc/init.d/growth_god

all: $(LOG_DIR) $(GOD_CONFIG_FILE) $(INIT_SCRIPT_FILE)

$(LOG_DIR):
	install -m 777 -d /var/log/growth

$(GOD_CONFIG_FILE): god/growth.god.conf
	install -D -m 644 $^ $@

$(INIT_SCRIPT_FILE): god/growth_god
	install -D -m 755 $^ $@
	update-rc.d growth_god defaults
