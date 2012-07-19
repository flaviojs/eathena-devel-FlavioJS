from twisted.internet import defer, reactor
from twisted.python import log
from buildbot.status.base import StatusReceiverMultiService


class ShutdownIdleSlaves(StatusReceiverMultiService):
	"""I am a status listener that shuts down target slaves if they are done."""

	def __init__(self, slaves):
		StatusReceiverMultiService.__init__(self)
		
		assert isinstance(slaves,list)
		for name in slaves:
			assert isinstance(name,str)
		
		self.slaves = slaves

	def setServiceParent(self, parent):
		StatusReceiverMultiService.setServiceParent(self, parent)
		
		self.master = parent.master
		s = self.master.getStatus()
		s.subscribe(self)

	def builderAdded(self, builderName, builder):
		"""Listen to all builders"""
		builder.subscribe(self)

	def builderChangedState(self, builderName, state):
		"""Check the slaves if the builder is done"""
		if state != "idle":
			return # is busy
		
		builder = self._getBuilder(builderName)
		d = builder.getPendingBuildRequestStatuses()
		def checkPending(results, self, builder):
			if len(results) > 0:
				return # has pending builds
			for slave in builder.getSlaves():
				self._maybeShutdownSlave(slave)
		d.addCallback(checkPending, self, builder)

	def _getBuilder(self, name):
		s = self.master.getStatus()
		builder = s.getBuilder(name)
		return builder

	def _getSlaveBuilders(self, name):
		s = self.master.getStatus()
		builders = []
		for bname in s.getBuilderNames():
			builder = s.getBuilder(bname)
			slavenames = [slave.getName() for slave in builder.getSlaves()]
			if name in slavenames:
				builders.append(builder)
		return builders

	def _getSlave(self, name):
		s = self.master.getStatus()
		slave = s.getSlave(name)
		return slave

	def _maybeShutdownSlave(self, slave):
		if slave.getName() not in self.slaves:
			return # not a target slave
		if slave.getGraceful():
			return # already shutting down
		
		builders = self._getSlaveBuilders(slave.getName())
		d = defer.gatherResults([builder.getPendingBuildRequestStatuses() for builder in builders])
		def checkResults(results, self, slave):
			if slave.getGraceful():
				return # already shutting down
			if len(slave.getRunningBuilds()) > 0:
				return # slave is building
			for pending in results:
				if len(pending) > 0:
					return # slave has pending builds
			log.msg("ShutdownIdleSlaves: initiating graceful shutdown for slave {}".format(slave.getName()))
			slave.setGraceful(True)
		d.addCallback(checkResults, self, slave)
		return d
