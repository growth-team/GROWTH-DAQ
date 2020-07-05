var casper = require('casper').create();
var url = 'http://admin:growth@192.168.179.1/index.cgi/reboot_main';

casper.start(url, function() {
    this.click('#UPDATE_BUTTON');
});

casper.then(function() {
    this.page.onAlert = function(message) {
    return true;
    };
});

casper.run();
