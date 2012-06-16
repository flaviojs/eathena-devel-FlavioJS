from buildbot.process.factory import BuildFactory

class CMakeFactory(BuildFactory):
    sourcedir = "source"

    def __init__ ( self, baseURL, useBuildType=None, arguments=[]):
        """Factory for CMake out-of-source builds with extra buildbot code.

        Uses the following builder properties (strings) when generating a build:
        - generator: CMake generator
        - arguments: build-specific CMake arguments

        @cvar baseURL: url of the svn repository
        @cvar useBuildType: for multi-configuration generators (see CMAKE_BUILD_TYPE)
        @cvar arguments: extra CMake arguments
        """
        assert baseURL is not None
        assert type(arguments) == type([])
        from buildbot.steps import source, slave, shell
        from buildbot.process.properties import WithProperties, Property
        BuildFactory.__init__(self)
        if useBuildType is None:
            buildTypeArgument=[]
        else:
            buildTypeArgument=["--config", useBuildType]

        # update svn
        self.addStep(source.SVN(
            workdir=self.sourcedir,
            mode='update',
            baseURL=baseURL,
            defaultBranch='trunk',
            retry=(30,2) ))
        # recreate build dir
        self.addStep(slave.RemoveDirectory(
            dir=self.workdir,
            haltOnFailure=False,
            flunkOnFailure=False))
        self.addStep(slave.MakeDirectory(
            dir=self.workdir))
        # configure
        self.addStep(shell.Configure(
            command=["cmake", "../source", WithProperties("-G%s", 'generator'), arguments, Property('extra_arguments', default=[])],
            logEnviron=False))
        # compile - the install target builds and then copies files to the
        # production directory (default is subdirectory 'install') and removes
        # full paths from library dependencies so it works when you copy the
        # production files elsewhere
        self.addStep(shell.Compile(
            command=["cmake", "--build", ".", "--target", "install"] + buildTypeArgument,
            logEnviron=False))
        # package - the package target creates an installer/package/archive
        # that contains the production files; the target is only available if 
        # CPack and a package generator are available
        self.addStep(shell.SetProperty(
            haltOnFailure = True,
            flunkOnFailure = True,
            extract_fn=lambda rc, stdout, stderr: {"WITH_CPACK": stdout.find("WITH_CPACK:BOOL=ON") != -1},
            command=["cmake", "-N", "-LA"],
            logEnviron=False))
        self.addStep(shell.ShellCommand(
            name = "Package",
            description = ["packaging"],
            descriptionDone = ["package"],
            haltOnFailure = False,
            flunkOnFailure = False,
            doStepIf=lambda step: step.build.getProperty("WITH_CPACK"),
            command=["cmake", "--build", ".", "--target", "package"] + buildTypeArgument,
            logEnviron=False))
