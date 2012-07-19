"""IRC bot that restricts access based on a configuration file.
Maintainer: Flavio J. Saraiva (feel free to send complaints or suggestions)
	flaviojs @ eAthena forum/irc
	flaviojs2005 \A-T/ gmail <D.o,T> com
"""
from twisted.application import internet
from twisted.internet import defer
from twisted.python import log, failure
from buildbot.status.words import have_ssl, IRC, IrcStatusFactory, IrcStatusBot, IRCContact
from buildbot.status.base import StatusReceiverMultiService


class ExtendedIRCContact(IRCContact):
	"""Extended IRCContact class.
	Changes:
	 1) fix functions act and handleAction
	 2) add command 'reload' to reload the user access data
	 3) make sure the user has access to the command he's trying to use
	"""

	def command_RELOAD(self, args, who):
		# XXX new command
		try:
			self.bot.parseAccess()
			self.send("Done")
		except:
			f = failure.Failure()
            		log.err(f)
			self.send("Something bad happened (see logs)")
	command_RELOAD.usage = "reload - reload the user access data"

	def act(self, action):
		# FIX use describe instead of me to avoid axception
		# XXX fixed in master, remove on next release
		if not self.muted:
			self.bot.describe(self.dest, action.encode("ascii", "replace"))

	def handleMessage(self, message, who):
		# XXX extended behaviour - make sure use has access to the command
		def handleMessageAccess(canAccess, self, message, who):
			if canAccess:
				IRCContact.handleMessage(self, message, who)
			else:
				self.send("%s: Sorry, you're not allowed to use this command" % who)
		message = message.lstrip()
		if self.silly.has_key(message):
			return self.doSilly(message)
		
		cmd = message.split(' ', 1)[0]
		meth = self.getCommandMethod(cmd)
		if meth:
			canAccess = self.bot.getAccess(who, cmd)
			if isinstance(canAccess, bool):
				handleMessageAccess(canAccess, self, message, who)
			else:# deferred
				canAccess.addCallback(handleMessageAccess, self, message, who)
				canAccess.callback(None)

	def handleAction(self, data, user):
		# FIX use the bot nickname instead of "buildbot"
		# XXX fixed in master, remove on next release
		if not data.endswith("s "+ self.bot.nickname):
			return
		words = data.split()
		verb = words[-2]
		if verb == "kicks":
			response = "%s back" % verb
		else:
			response = "%s %s too" % (verb, user)
		self.act(response)


class ExtendedIrcStatusBot(IrcStatusBot):
	"""Extended IrcStatusBot class.
	Changes:
	 1) use ExtendedIRCContat for the contact class
	 2) fix functions getContact, privmsg and action
	 3) read access file
	 4) check user access and, if needed, ask Nickserv if the user is registered and authenticated
	"""
	contactClass = ExtendedIRCContact
	access_file = "ircaccess.cfg"

	def parseAccess(self):
		# XXX new method
		access = {}# dict of user->(dict of command->canAccess)
		data = None
		with open(self.access_file) as f: data = eval(f.read())
		for user in data.keys():
			userAccess = data[user]
			user = user.lower()# users are case insensitive
			assert access.has_key(user) == False# must be unique
			access[user] = {}
			for command in userAccess.keys():
				canAccess = userAccess[command]
				command = command.lower()# commands are case insensitive
				assert canAccess in (True,False)# must be boolean
				assert access[user].has_key(command) == False# must be unique
				access[user][command] = canAccess
		if hasattr(self,"access_deferred"):
			for user in self.access_deferred.keys():
				d = self.access_deferred[user]
				d.errback(Exception("AuthIrcStatusBot.access_deferred['%s'] interrupted while reloading access file" % user))
		self.access_deferred = {}# dict of user->deferred
		self.access = access

	def statusReceived(self, user, status):
		# XXX overload empty method
		"""Called when a status check is received."""
		user = user.lower()
		if self.access_deferred.has_key(user):
			d = self.access_deferred[user]
			del self.access_deferred[user]
			d.callback(int(status) == 3)# True if registered and authenticated

	def getAccess(self, user, command):
		# XXX new method
		"""Checks if the user has access to the command.
		Returns True if the command is allowed.
		Returns False if the command is not allowed.
		Returns a deferred if aditional checks are needed to know if the command is allowed.
		"""
		user = user.lower()
		command = command.lower()
		
		auth_access = None
		if self.access.has_key(user):
			userAccess = self.access[user]
			def checkRegistered(_, self, user):
				"""Checks if the user is registered and authenticated.
				Returns contact,True if registered and authenticated.
				Returns contact,False in not registered adn authenticated.
				Returns contact,None if interrupted.
				"""
				if self.access_deferred.has_key(user):
					self.access_deferred[user].errback(Exception("AuthIrcStatusBot.access_deferred['%s'] interrupted while getting a new access" % user))
				d = defer.Deferred()
				self.access_deferred[user] = d
				self.msg("Nickserv", "STATUS %s" % user)
				return d
			def deferredAccess():
				"""When the deferred is called, starts checking if a user is registered and authenticated."""
				d = defer.Deferred()
				d.addCallback(checkRegistered, self, user)
				return d
			if userAccess.has_key(command):# user.command
				if userAccess[command]:
					auth_access = deferredAccess()
				else:
					auth_access = False
			elif userAccess.has_key("*"):# user.*
				if userAccess["*"]:
					auth_access = deferredAccess()
				else:
					auth_access = False
		anon_access = False# dissalow by default
		if self.access.has_key("*"):
			defaultAccess = self.access["*"]
			if defaultAccess.has_key(command):# *.command
				anon_access = defaultAccess[command]
			elif defaultAccess.has_key("*"):# *.*
				anon_access = defaultAccess["*"]
		if auth_access is None:
			return anon_access# anonymous access
		if auth_access and anon_access:
			return True# can access while authenticated and anonymous, so skip auth check
		return auth_access# do auth check if needed

	def getContact(self, name):
		# FIX nicknames and channels are case insensitive
		# XXX fixed in master, remove on next release
		name = name.lower()
		return IrcStatusBot.getContact(self, name)

	def privmsg(self, user, channel, message):
		# FIX fail to private message if the nickname has uppercase letters
		# XXX fixed in master, remove on next release
		user = user.split('!', 1)[0]
		if channel == self.nickname:
			contact = self.getContact(user)
			contact.handleMessage(message, user)
			return
		contact = self.getContact(channel)
		if message.startswith("%s:" % self.nickname) or message.startswith("%s," % self.nickname):
			message = message[len("%s:" % self.nickname):]
			contact.handleMessage(message, user)

	def action(self, user, channel, data):
		# FIX use the bot nickname instead of "builbot"
		# XXX fixed in master, remove on next release
		user = user.split('!', 1)[0]
		contact = self.getContact(channel)
		if self.nickname.lower() in data.lower():
			contact.handleAction(data, user)

	def noticed(self, user, channel, message):
		# XXX overload empty method
		user = user.split('!', 1)[0]
		words = message.split()
		if channel == self.nickname and user.lower() == "NickServ".lower():
			if len(words) == 3 and words[0] == "STATUS":
				self.statusReceived(words[1], words[2])

	def signedOn(self):
		# XXX overload empty method
		IrcStatusBot.signedOn(self)
		self.mode(self.nickname, True, "isxBRT")
		self.parseAccess()


class ExtendedIrcStatusFactory(IrcStatusFactory):
	"""Extended IrcStatusFactory class.
	Changes:
	 1) use ExtendedIrcStatusBot for the protocol class
	 3) send empty quit message when stopping the factory and not reconfiguring
	"""
	protocol = ExtendedIrcStatusBot

	def stopFactory(self):
		# XXX overload empty method
		"""Send a quit message before leaving."""
		if not self.shuttingDown and self.p:
			self.p.quit("") # TODO does nothing because this method is called after the socket is closed


class ExtendedIRC(IRC):
	"""Extended IRC class.
	Changes:
	 1) use ExtendedIrcStatusBot for the factory class
	"""
	factory = ExtendedIrcStatusFactory
	
	def __init__(self, host, nick, channels, pm_to_nicks=[], port=6667,
			allowForce=False, categories=None, password=None, notify_events={},
			noticeOnChannel = False, showBlameList = True, useRevisions=False,
			useSSL=False, lostDelay=None, failedDelay=None, useColors=True):
		# XXX replace method - use class in the factory variable
		StatusReceiverMultiService.__init__(self)
		
		assert allowForce in (True, False)
		
		self.host = host
		self.port = port
		self.nick = nick
		self.channels = channels
		self.pm_to_nicks = pm_to_nicks
		self.password = password
		self.allowForce = allowForce
		self.useRevisions = useRevisions
		self.categories = categories
		self.notify_events = notify_events
		
		self.f = self.factory(self.nick, self.password,
			self.channels, self.pm_to_nicks,
			self.categories, self.notify_events,
			noticeOnChannel = noticeOnChannel,
			useRevisions = useRevisions,
			showBlameList = showBlameList,
			lostDelay = lostDelay,
			failedDelay = failedDelay,
			useColors = useColors)
		
		if useSSL:
			if not have_ssl:
				raise RuntimeError("useSSL requires PyOpenSSL")
			from twisted.internet import ssl
			cf = ssl.ClientContextFactory()
			c = internet.SSLClient(self.host, self.port, self.f, cf)
		else:
			c = internet.TCPClient(self.host, self.port, self.f)
		
		c.setServiceParent(self)
