"""IRC bot that restricts access based on a configuration file.
Maintainer: Flavio J. Saraiva (feel free to send complaints or suggestions)
	flaviojs @ eAthena forum/irc
	flaviojs2005 \A-T/ gmail <D.o,T> com
"""
from twisted.internet import defer
from twisted.python import log, failure
from buildbot.status.words import IrcStatusBot, IRCContact


class AuthIRCContact(IRCContact):
	"""Extended IRCContact class.
	Changes:
	 1) fix functions act and handleAction
	 2) add command 'reload' to reload the user access data
	 3) make sure the user has access to the command he's trying to use
	"""

	def command_RELOAD(self, args, who):
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
		#self.bot.log("DEBUG handleMessage [%s] [%s]" % (message, who))
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
		#self.bot.log("DEBUG handleAction [%s] [%s]" % (data, user))
		if not data.endswith("s "+ self.bot.nickname):
			return
		words = data.split()
		verb = words[-2]
		if verb == "kicks":
			response = "%s back" % verb
		else:
			response = "%s %s too" % (verb, user)
		self.act(response)


class AuthIrcStatusBot(IrcStatusBot):
	"""Extended IrcStatusBot class.
	Changes:
	 1) fix functions getContact, privmsg and action
	 2) read access file
	 3) check user access and, if needed, ask Nickserv if the user is registered and authenticated
	"""
	contactClass = AuthIRCContact
	access_file = "ircaccess.cfg"

	def parseAccess(self):
		self.access = {}# dict of user->(dict of command->canAccess)
		if hasattr(self,"access_deferred"):
			for user in self.access_deferred.keys():
				d = self.access_deferred[user]
				d.errback(Exception("AuthIrcStatusBot.access_deferred['%s'] interrupted while reloading access file" % user))
		self.access_deferred = {}# dict of user->deferred
		data = None
		with open(self.access_file) as f: data = eval(f.read())
		for user in data.keys():
			userAccess = data[user]
			user = user.lower()# users are case insensitive
			assert self.access.has_key(user) == False# must be unique
			self.access[user] = {}
			for command in userAccess.keys():
				canAccess = userAccess[command]
				command = command.lower()# commands are case insensitive
				assert canAccess in (True,False)# must be boolean
				assert self.access[user].has_key(command) == False# must be unique
				self.access[user][command] = canAccess

	def statusReceived(self, user, status):
		"""Called when a status check is received."""
		user = user.lower()
		if self.access_deferred.has_key(user):
			d = self.access_deferred[user]
			del self.access_deferred[user]
			d.callback(int(status) == 3)# True if registered and authenticated

	def getAccess(self, user, command):
		"""Checks if the user has access to the command.
		Returns True if the command is allowed.
		Returns False if the command is not allowed.
		Returns a deferred if aditional checks are needed to know if the command is allowed.
		"""
		user = user.lower()
		command = command.lower()
		
		if self.access.has_key(user):
			userAccess = self.access[user]
			def checkRegistered(_, self, user):
				"""Checks if the user is registered.
				Returns contact,True if registered.
				Returns contact,False in not registered.
				Returns contact,None if interrupted.
				"""
				if self.access_deferred.has_key(user):
					self.access_deferred[user].errback(Exception("AuthIrcStatusBot.access_deferred['%s'] interrupted while getting a new access" % user))
				d = defer.Deferred()
				self.access_deferred[user] = d
				self.msg("Nickserv", "STATUS %s" % user)
				return d
			def deferredAccess():
				"""When called, starts checking if a user is registered."""
				d = defer.Deferred()
				d.addCallback(checkRegistered, self, user)
				return d
			if userAccess.has_key(command):# user.command
				return deferredAccess() if userAccess[command] else False
			if userAccess.has_key("*"):# user.*
				return deferredAccess() if userAccess["*"] else False
		if self.access.has_key("*"):
			defaultAccess = self.access["*"]
			if defaultAccess.has_key(command):# *.command
				return defaultAccess[command]
			if defaultAccess.has_key("*"):# *.*
				return defaultAccess["*"]
		return False# dissalow by default

	def getContact(self, name):
		# FIX nicknames and channels are case insensitive
		#self.log("DEBUG getContact [%s]" % name)
		name = name.lower()
		return IrcStatusBot.getContact(self, name)

	def privmsg(self, user, channel, message):
		# FIX fail to private message if the nickname has uppercase letters
		#self.log("DEBUG privmsg [%s] [%s] [%s]" % (user, channel, message))
		user = user.split('!', 1)[0]
		if channel == self.nickname:
			contact = self.getContact(user)
			contact.handleMessage(message, user)
			return
		contact = self.getContact(channel)
		if message.startswith("%s:" % self.nickname) or message.startswith("%s," % self.nickname):
			message = message[len("%s:" % self.nickname):]
			contact.handleMessage(message, user)
		#IrcStatusBot.privmsg(self, user, channel, data)

	def action(self, user, channel, data):
		# FIX use the bot nickname instead of "builbot"
		#self.log("DEBUG action [%s] [%s] [%s]" % (user, channel, data))
		user = user.split('!', 1)[0]
		contact = self.getContact(channel)
		if self.nickname.lower() in data.lower():
			contact.handleAction(data, user)

	def noticed(self, user, channel, message):
		user = user.split('!', 1)[0]
		words = message.split()
		if channel == self.nickname and user.lower() == "NickServ".lower():
			if len(words) == 3 and words[0] == "STATUS":
				self.statusReceived(words[1], words[2])

	def signedOn(self):
		IrcStatusBot.signedOn(self)
		self.mode(self.nickname, True, "isxBRT")
		self.parseAccess()


def attachAuthIrcStatusBot(irc):
	""" Receives an instance of IRC and attaches AuthIrcStatusBot as the factory protocol."""
	irc.f.protocol = AuthIrcStatusBot
