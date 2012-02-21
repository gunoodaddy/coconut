#!/usr/local/python/bin/python
from twisted.internet.protocol	import Protocol, Factory
from twisted.internet			import epollreactor, defer
epollreactor.install()
from twisted.internet			import reactor

class Echo(Protocol):
	def connectionMade(self):
		#print 'CONNECTED'
		pass
	
	def dataReceived(self, data):
		"""
        As soon as any data is received, write it back.
		"""
		#print data
		self.transport.write(data)

def main():
    f = Factory()
    f.protocol = Echo
    reactor.listenTCP(3333, f)
    reactor.run()

if __name__ == '__main__':
    main()
