# -*- python -*-
# ex: set syntax=python:

# This is a sample buildmaster config file. It must be installed as
# 'master.cfg' in your buildmaster's base directory.

# This is the dictionary that the buildmaster pays attention to. We also use
# a shorter alias to save typing.
c = BuildmasterConfig = {}

####### BUILDSLAVES

# The 'slaves' list defines the set of recognized buildslaves. Each element is
# a BuildSlave object, specifying a unique slave name and password.  The same
# slave name and password must be configured on the slave.

from buildbot.buildslave import BuildSlave
#from buildbot.locks import SlaveLock
#lock_flaviojs = SlaveLock("lock-flaviojs") # TODO slave locks are bugged on buildbot 0.8.6p1, retry on the next version
c['slaves'] = [
	BuildSlave("eAthena-Ubuntu-12.04-x64", "%%EATHENA-SLAVE-PASSWORD%%"), # builds and tests
	BuildSlave("Paradox924X-Ubuntu-10.10", "%%PARADOX924X-SLAVE-PASSWORD%%", max_builds=1),
	BuildSlave("Ai4rei-Debian-6.0-x64", "%%AI4REI-SLAVE-PASSWORD%%", max_builds=1),
	BuildSlave("flaviojs-Cygwin", "%%FLAVIOJS-SLAVE-PASSWORD%%", max_builds=1),
	BuildSlave("flaviojs-Windows-MinGW", "%%FLAVIOJS-SLAVE-PASSWORD%%", max_builds=1),
	BuildSlave("flaviojs-Windows-MSYS", "%%FLAVIOJS-SLAVE-PASSWORD%%", max_builds=1),
	BuildSlave("flaviojs-Windows-VS10", "%%FLAVIOJS-SLAVE-PASSWORD%%", max_builds=1),
	BuildSlave("flaviojs-Windows-VS10-x64", "%%FLAVIOJS-SLAVE-PASSWORD%%", max_builds=1)]

# 'slavePortnum' defines the TCP port to listen on for connections from slaves.
# This must match the value configured into the buildslaves (with their
# --master option)
c['slavePortnum'] = 9989

c['mergeRequests'] = False

####### CHANGESOURCES

# the 'change_source' setting tells the buildmaster how it should find out
# about source code changes.  Here we point to the buildbot clone of pyflakes.

ea_svnurl = "http://eathena-project.googlecode.com/svn/"
def ea_split_file_branches(path):
	pieces = path.split('/')
	if len(pieces) > 1 and pieces[0] == 'trunk':
		return (None, '/'.join(pieces[1:]))
	elif len(pieces) > 2 and pieces[0] == 'branches':
		return ('/'.join(pieces[0:2]), '/'.join(pieces[2:]))
	else:
		return None

from buildbot.changes.svnpoller import SVNPoller

c['change_source'] = []
c['change_source'].append(SVNPoller(
	svnurl = ea_svnurl,
	split_file = ea_split_file_branches,
	pollInterval = 60, # check every minute
	histmax = 10, # check up to 10 revisions
	revlinktmpl = "http://code.google.com/p/eathena-project/source/detail?r=%s"))

####### SCHEDULERS

# Configure the Schedulers, which decide how to react to incoming changes.  In this
# case, just kick off a 'runtests' build

from buildbot.schedulers.triggerable import Triggerable
from buildbot.schedulers.basic import AnyBranchScheduler
from buildbot.schedulers.forcesched import ForceScheduler
from buildbot.schedulers.trysched import Try_Userpass
from buildbot.changes import filter

def ea_should_build(change):
	for path in change.files:
		pieces = path.split('/')
		if len(pieces) <= 1 and path.find("Changelog") == 0:
			return True # not inside a folder and not the changelog, assume we need to build
		elif pieces[0] == "3rdparty" or pieces[0] == "src" or pieces[0].find("vcproj-") == 0:
			return True # inside a folder related to builds, 
	return False

c['schedulers'] = []
c['schedulers'].append(AnyBranchScheduler(
	name = "build-scheduler",
	treeStableTimer = None,
	builderNames = [
		"CMake-Ubuntu-10.10", "Ubuntu-10.10",
		"CMake-Ubuntu-12.04-x64", "Ubuntu-12.04-x64",
		#"CMake-Debian-6.0-x64", # TODO try to lower the requirement to cmake 2.8.2
		"Debian-6.0-x64",
		"CMake-Cygwin", "Cygwin",
		"CMake-Windows-MinGW", "CMake-Windows-MSYS",
		"CMake-Windows-NMake-VS10", "CMake-Windows-VS10", "Windows-VS10",
		"CMake-Windows-NMake-VS10-x64", "CMake-Windows-VS10-x64",
		],
	fileIsImportant = ea_should_build,
	onlyImportant = True))
c['schedulers'].append(Triggerable(
	name = "test-Ubuntu-12.04-x64-scheduler",
	builderNames = ["test-Ubuntu-12.04-x64"]))
#c['schedulers'].append(AnyBranchScheduler(
#	name = "test-scheduler",
#	treeStableTimer = None,
#	builderNames = [
#		"test-Ubuntu-12.04-x64",
#		]))
c['schedulers'].append(Try_Userpass(
	name = "try-scheduler",
	builderNames = ["Ubuntu-12.04-x64"],
	port = 8031,
	userpass = [("%%TRY-USERNAME%%","%%TRY-PASSWORD%%")]))

####### BUILDERS

# The 'builders' list defines the Builders, which tell Buildbot how to perform a build:
# what steps, and which slaves can execute them.  Note that any particular build will
# only take place on one slave.

from buildbot.steps.shell import ShellCommand,Configure,Compile,Test
from buildbot.steps.source import SVN
from buildbot.steps.vstudio import VC10
from buildbot.steps.trigger import Trigger
from buildbot.process.factory import BuildFactory
from cmake import CMakeFactory

step_svn_copy = SVN(
	mode = 'copy',
	baseURL = (ea_svnurl + '%%BRANCH%%'),
	defaultBranch = 'trunk',
	retry = (30,2),
	logEnviron = True)
step_autoconf = Compile(
	command = ["autoconf"],
	description = ["running autoconf"],
	descriptionDone = ["autoconf"],
	logEnviron = False)
step_configure = Configure(
	command = ["./configure"],
	logEnviron = False)
step_configure_64 = Configure(
	command = ["./configure", "--enable-64bit"],
	logEnviron = False)
step_compile_all = Compile(
	command = ["make", "clean", "all"],
	logEnviron = False)
step_compile_txt = Compile(
	command = ["make", "clean", "txt"],
	description = "compiling txt",
	descriptionDone = "compile txt",
	logEnviron = False)
step_compile_sql = Compile(
	command = ["make", "clean", "sql"],
	description = "compiling sql",
	descriptionDone = "compile sql",
	logEnviron = False)
step_compile_VS10 = Compile(
	command = ["devenv.com", "eAthena-10.sln", "/REBUILD"],
	logEnviron = False)
step_trigger_tests = Trigger(
	waitForFinish = True,
	schedulerNames=["test-Ubuntu-12.04-x64-scheduler"])
step_test_txt = Test(
	command = ["gdb", "map-server", "-ex=run --run-once", "-ex=bt full", "-ex=kill", "-ex=quit"],
	warningPattern = "\[(Error|Warning)\]",
	description = "testing txt",
	descriptionDone = "test txt",
	logEnviron = False)
step_test_sql = Test(
	command = ["gdb", "map-server_sql", "-ex=run --run-once", "-ex=bt full", "-ex=kill", "-ex=quit"],
	warningPattern = "\[(Error|Warning)\]",
	description = "testing sql",
	descriptionDone = "test sql",
	logEnviron = False)
f_unix = BuildFactory(steps = [step_svn_copy, step_configure, step_compile_all])
f_unix_64 = BuildFactory(steps = [step_svn_copy, step_configure_64, step_compile_all])
f_unix_64_trigger = BuildFactory(steps = [step_svn_copy, step_configure_64, step_compile_all, step_trigger_tests])
f_unix_64_tests = BuildFactory(steps = [step_test_txt])
f_VS10 = BuildFactory(steps = [step_svn_copy, step_compile_VS10])
f_CMake_single = CMakeFactory(
	baseURL = (ea_svnurl + '%%BRANCH%%'),
	arguments = ["-DCMAKE_BUILD_TYPE=RelWithDebInfo"])
f_CMake_multi = CMakeFactory(
	baseURL = (ea_svnurl + '%%BRANCH%%'),
	useBuildType = "RelWithDebInfo")
p_extra_arguments = ["-DBUILD_PLUGIN_console=ON", "-DBUILD_PLUGIN_pid=ON", "-DBUILD_PLUGIN_sample=ON", "-DBUILD_PLUGIN_sig=ON"]
p_extra_arguments_VS10 = ["-DBUILD_PLUGIN_console=ON", "-DBUILD_PLUGIN_dbghelpplug=ON", "-DBUILD_PLUGIN_pid=ON", "-DBUILD_PLUGIN_sample=ON", "-DBUILD_PLUGIN_sig=ON"]
p_CMake_UnixMakefiles = {
	"generator": "Unix Makefiles",
	"extra_arguments": p_extra_arguments}

from buildbot.config import BuilderConfig
c['builders'] = []

# Linux builders (32 bit)
c['builders'].append(BuilderConfig(
	category = "build",
	name = "Ubuntu-10.10",
	slavenames = ["Paradox924X-Ubuntu-10.10"],
	factory = f_unix))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Ubuntu-10.10",
	slavenames = ["Paradox924X-Ubuntu-10.10"],
	factory = f_CMake_single,
	properties = {
		"generator": "Unix Makefiles",
		"extra_arguments": p_extra_arguments}))

# Linux builders (64 bit)
c['builders'].append(BuilderConfig(
	category = "build",
	name = "Ubuntu-12.04-x64",
	slavebuilddir = "Ubuntu-12_04-x64",
	slavenames = ["eAthena-Ubuntu-12.04-x64"],
	factory = f_unix_64_trigger))
c['builders'].append(BuilderConfig(
	category = "test",
	name = "test-Ubuntu-12.04-x64",
	slavebuilddir = "Ubuntu-12_04-x64",
	slavenames = ["eAthena-Ubuntu-12.04-x64"],
	factory = f_unix_64_tests))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Ubuntu-12.04-x64",
	slavenames = ["eAthena-Ubuntu-12.04-x64"],
	factory = f_CMake_single,
	properties = {
		"generator": "Unix Makefiles",
		"extra_arguments": p_extra_arguments}))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "Debian-6.0-x64",
	slavenames = ["Ai4rei-Debian-6.0-x64"],
	factory = f_unix_64))
#c['builders'].append(BuilderConfig( # TODO try to lower the requirement to cmake 2.8.2
#	category = "build",
#	name = "CMake-Debian-6.0-x64",
#	slavenames = ["Ai4rei-Debian-6.0-x64"],
#	factory = f_CMake_single,
#	properties = {
#		"generator": "Unix Makefiles",
#		"extra_arguments": p_extra_arguments}))

# Cygwin builders (32 bit)
c['builders'].append(BuilderConfig(
	category = "build",
	name = "Cygwin",
	slavenames = ["flaviojs-Cygwin"],
	factory = f_unix))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Cygwin",
	slavenames = ["flaviojs-Cygwin"],
	factory = f_CMake_single,
	properties = {
		"generator": "Unix Makefiles",
		"extra_arguments": p_extra_arguments}))

# Windows builders (32 bit)
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Windows-MinGW",
	slavenames = ["flaviojs-Windows-MinGW"],
	factory = f_CMake_single,
	properties = {
		"generator": "MinGW Makefiles",
		"extra_arguments": p_extra_arguments}))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Windows-MSYS",
	slavenames = ["flaviojs-Windows-MSYS"],
	factory = f_CMake_single,
	properties = {
		"generator": "MSYS Makefiles",
		"extra_arguments": p_extra_arguments}))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "Windows-VS10",
	slavenames = ["flaviojs-Windows-VS10"],
	factory = f_VS10))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Windows-VS10",
	slavenames = ["flaviojs-Windows-VS10"],
	factory = f_CMake_multi,
	properties = {
		"generator": "Visual Studio 10",
		"extra_arguments": p_extra_arguments_VS10}))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Windows-NMake-VS10",
	slavenames = ["flaviojs-Windows-VS10"],
	factory = f_CMake_single,
	properties = {
		"generator": "NMake Makefiles",
		"extra_arguments": p_extra_arguments_VS10}))

# Windows builders (64 bit)
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Windows-VS10-x64",
	slavenames = ["flaviojs-Windows-VS10-x64"],
	factory = f_CMake_multi,
	properties = {
		"generator": "Visual Studio 10 Win64",
		"extra_arguments": p_extra_arguments}))
c['builders'].append(BuilderConfig(
	category = "build",
	name = "CMake-Windows-NMake-VS10-x64",
	slavenames = ["flaviojs-Windows-VS10-x64"],
	factory = f_CMake_single,
	properties = {
		"generator": "NMake Makefiles",
		"extra_arguments": p_extra_arguments}))

####### STATUS TARGETS

# 'status' is a list of Status Targets. The results of each build will be
# pushed to these targets. buildbot/status/*.py has a variety to choose from,
# including web pages, email senders, and IRC bots.


from buildbot.status import html
from buildbot.status.web import authz, auth
from irc import ExtendedIRC
from nightly import ShutdownIdleSlaves

authz_cfg = authz.Authz(
	auth = auth.BasicAuth([("%%WEB-USERNAME%%","%%WEB-PASSWORD%%")]),
	default_action = 'auth')
status_web = html.WebStatus(
	authz = authz_cfg,
	http_port = 8010)
status_irc = ExtendedIRC(
	host = "irc.dairc.net",
	port = 6697,
	useSSL = True,
	useRevisions = True,
	nick = "eA-BuildBot",
	password = "%%IRC-BOT-PASSWORD%%",
	channels = ["#Athena"],
	notify_events = {
		#'started': 1,
		#'finished': 1,
		#'success': 1,
		'failure': 1,
		},
	allowForce = True)
status_shutdown = ShutdownIdleSlaves(
	slaves = [
		"flaviojs-Cygwin",
		"flaviojs-Windows-MinGW",
		"flaviojs-Windows-MSYS",
		"flaviojs-Windows-VS10",
		"flaviojs-Windows-VS10-x64"])

c['status'] = [status_web, status_irc, status_shutdown]

####### PROJECT IDENTITY

# the 'title' string will appear at the top of this buildbot
# installation's html.WebStatus home page (linked to the
# 'titleURL') and is embedded in the title of the waterfall HTML page.

c['title'] = "eAthena"
c['titleURL'] = "http://www.eathena.ws/"

# the 'buildbotURL' string should point to the location where the buildbot's
# internal web server (usually the html.WebStatus page) is visible. This
# typically uses the port number set in the Waterfall 'status' entry, but
# with an externally-visible host name which the buildbot cannot figure out
# without some help.

c['buildbotURL'] = "http://srv.eathena.ws:8010/"

####### DB URL

c['db'] = {
    # This specifies what database buildbot uses to store its state.  You can leave
    # this at its default for all but the largest installations.
    'db_url' : "sqlite:///state.sqlite",
}
