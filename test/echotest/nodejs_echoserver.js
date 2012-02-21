var net = require('net');

var server = net.createServer(function (client) {
	/*
	client.write("Hello\r\n");
	*/
	client.on('data', function(data) {
		client.write(data);
	});
});

server.listen(2222, "0.0.0.0");
