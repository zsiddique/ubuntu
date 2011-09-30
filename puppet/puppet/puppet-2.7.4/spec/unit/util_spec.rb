#!/usr/bin/env ruby

require 'spec_helper'

describe Puppet::Util do
  describe "#absolute_path?" do
    it "should default to the platform of the local system" do
      Puppet.features.stubs(:posix?).returns(true)
      Puppet.features.stubs(:microsoft_windows?).returns(false)

      Puppet::Util.should be_absolute_path('/foo')
      Puppet::Util.should_not be_absolute_path('C:/foo')

      Puppet.features.stubs(:posix?).returns(false)
      Puppet.features.stubs(:microsoft_windows?).returns(true)

      Puppet::Util.should be_absolute_path('C:/foo')
      Puppet::Util.should_not be_absolute_path('/foo')
    end

    describe "when using platform :posix" do
      %w[/ /foo /foo/../bar //foo //Server/Foo/Bar //?/C:/foo/bar /\Server/Foo].each do |path|
        it "should return true for #{path}" do
          Puppet::Util.should be_absolute_path(path, :posix)
        end
      end

      %w[. ./foo \foo C:/foo \\Server\Foo\Bar \\?\C:\foo\bar \/?/foo\bar \/Server/foo].each do |path|
        it "should return false for #{path}" do
          Puppet::Util.should_not be_absolute_path(path, :posix)
        end
      end
    end

    describe "when using platform :windows" do
      %w[C:/foo C:\foo \\\\Server\Foo\Bar \\\\?\C:\foo\bar //Server/Foo/Bar //?/C:/foo/bar /\?\C:/foo\bar \/Server\Foo/Bar].each do |path|
        it "should return true for #{path}" do
          Puppet::Util.should be_absolute_path(path, :windows)
        end
      end

      %w[/ . ./foo \foo /foo /foo/../bar //foo C:foo/bar].each do |path|
        it "should return false for #{path}" do
          Puppet::Util.should_not be_absolute_path(path, :windows)
        end
      end
    end
  end

  describe "execution methods" do
    let(:pid) { 5501 }
    let(:null_file) { Puppet.features.microsoft_windows? ? 'NUL' : '/dev/null' }

    describe "#execute_posix" do
      before :each do
        # Most of the things this method does are bad to do during specs. :/
        Kernel.stubs(:fork).returns(pid).yields
        Process.stubs(:setsid)
        Kernel.stubs(:exec)
        Puppet::Util::SUIDManager.stubs(:change_user)
        Puppet::Util::SUIDManager.stubs(:change_group)

        $stdin.stubs(:reopen)
        $stdout.stubs(:reopen)
        $stderr.stubs(:reopen)

        @stdin  = File.open(null_file, 'r')
        @stdout = Tempfile.new('stdout')
        @stderr = File.open(null_file, 'w')
      end

      it "should fork a child process to execute the command" do
        Kernel.expects(:fork).returns(pid).yields
        Kernel.expects(:exec).with('test command')

        Puppet::Util.execute_posix('test command', {}, @stdin, @stdout, @stderr)
      end

      it "should start a new session group" do
        Process.expects(:setsid)

        Puppet::Util.execute_posix('test command', {}, @stdin, @stdout, @stderr)
      end

      it "should close all open file descriptors except stdin/stdout/stderr" do
        # This is ugly, but I can't really think of a better way to do it without
        # letting it actually close fds, which seems risky
        (0..2).each {|n| IO.expects(:new).with(n).never}
        (3..256).each {|n| IO.expects(:new).with(n).returns mock('io', :close) }

        Puppet::Util.execute_posix('test command', {}, @stdin, @stdout, @stderr)
      end

      it "should permanently change to the correct user and group if specified" do
        Puppet::Util::SUIDManager.expects(:change_group).with(55, true)
        Puppet::Util::SUIDManager.expects(:change_user).with(50, true)

        Puppet::Util.execute_posix('test command', {:uid => 50, :gid => 55}, @stdin, @stdout, @stderr)
      end

      it "should exit failure if there is a problem execing the command" do
        Kernel.expects(:exec).with('test command').raises("failed to execute!")
        Puppet::Util.stubs(:puts)
        Puppet::Util.expects(:exit!).with(1)

        Puppet::Util.execute_posix('test command', {}, @stdin, @stdout, @stderr)
      end

      it "should properly execute commands specified as arrays" do
        Kernel.expects(:exec).with('test command', 'with', 'arguments')

        Puppet::Util.execute_posix(['test command', 'with', 'arguments'], {:uid => 50, :gid => 55}, @stdin, @stdout, @stderr)
      end

      it "should return the pid of the child process" do
        Puppet::Util.execute_posix('test command', {}, @stdin, @stdout, @stderr).should == pid
      end
    end

    describe "#execute_windows" do
      let(:proc_info_stub) { stub 'processinfo', :process_id => pid }

      before :each do
        Process.stubs(:create).returns(proc_info_stub)
        Process.stubs(:waitpid2).with(pid).returns([pid, 0])

        @stdin  = File.open(null_file, 'r')
        @stdout = Tempfile.new('stdout')
        @stderr = File.open(null_file, 'w')
      end

      it "should create a new process for the command" do
        Process.expects(:create).with(
          :command_line => "test command",
          :startup_info => {:stdin => @stdin, :stdout => @stdout, :stderr => @stderr}
        ).returns(proc_info_stub)

        Puppet::Util.execute_windows('test command', {}, @stdin, @stdout, @stderr)
      end

      it "should return the pid of the child process" do
        Puppet::Util.execute_windows('test command', {}, @stdin, @stdout, @stderr).should == pid
      end

      it "should quote arguments containing spaces if command is specified as an array" do
        Process.expects(:create).with do |args|
          args[:command_line] == '"test command" with some "arguments \"with spaces"'
        end.returns(proc_info_stub)

        Puppet::Util.execute_windows(['test command', 'with', 'some', 'arguments "with spaces'], {}, @stdin, @stdout, @stderr)
      end
    end

    describe "#execute" do
      before :each do
        Process.stubs(:waitpid2).with(pid).returns([pid, 0])
      end

      describe "when an execution stub is specified" do
        before :each do
          Puppet::Util::ExecutionStub.set do |command,args,stdin,stdout,stderr|
            "execution stub output"
          end
        end

        it "should call the block on the stub" do
          Puppet::Util.execute("/usr/bin/run_my_execute_stub").should == "execution stub output"
        end

        it "should not actually execute anything" do
          Puppet::Util.expects(:execute_posix).never
          Puppet::Util.expects(:execute_windows).never

          Puppet::Util.execute("/usr/bin/run_my_execute_stub")
        end
      end

      describe "when setting up input and output files" do
        include PuppetSpec::Files
        let(:executor) { Puppet.features.microsoft_windows? ? 'execute_windows' : 'execute_posix' }

        before :each do
          Puppet::Util.stubs(:wait_for_output)
        end

        it "should set stdin to the stdinfile if specified" do
          input = tmpfile('stdin')
          FileUtils.touch(input)

          Puppet::Util.expects(executor).with do |_,_,stdin,_,_|
            stdin.path == input
          end.returns(pid)

          Puppet::Util.execute('test command', :stdinfile => input)
        end

        it "should set stdin to the null file if not specified" do
          Puppet::Util.expects(executor).with do |_,_,stdin,_,_|
            stdin.path == null_file
          end.returns(pid)

          Puppet::Util.execute('test command')
        end

        describe "when squelch is set" do
          it "should set stdout and stderr to the null file" do
            Puppet::Util.expects(executor).with do |_,_,_,stdout,stderr|
              stdout.path == null_file and stderr.path == null_file
            end.returns(pid)

            Puppet::Util.execute('test command', :squelch => true)
          end
        end

        describe "when squelch is not set" do
          it "should set stdout to a temporary output file" do
            outfile = Tempfile.new('stdout')
            Tempfile.stubs(:new).returns(outfile)

            Puppet::Util.expects(executor).with do |_,_,_,stdout,_|
              stdout.path == outfile.path
            end.returns(pid)

            Puppet::Util.execute('test command', :squelch => false)
          end

          it "should set stderr to the same file as stdout if combine is true" do
            outfile = Tempfile.new('stdout')
            Tempfile.stubs(:new).returns(outfile)

            Puppet::Util.expects(executor).with do |_,_,_,stdout,stderr|
              stdout.path == outfile.path and stderr.path == outfile.path
            end.returns(pid)

            Puppet::Util.execute('test command', :squelch => false, :combine => true)
          end

          it "should set stderr to the null device if combine is false" do
            outfile = Tempfile.new('stdout')
            Tempfile.stubs(:new).returns(outfile)

            Puppet::Util.expects(executor).with do |_,_,_,stdout,stderr|
              stdout.path == outfile.path and stderr.path == null_file
            end.returns(pid)

            Puppet::Util.execute('test command', :squelch => false, :combine => false)
          end
        end
      end
    end

    describe "after execution" do
      let(:executor) { Puppet.features.microsoft_windows? ? 'execute_windows' : 'execute_posix' }
      before :each do
        Process.stubs(:waitpid2).with(pid).returns([pid, 0])

        Puppet::Util.stubs(executor).returns(pid)
      end

      it "should wait for the child process to exit" do
        Puppet::Util.stubs(:wait_for_output)

        Process.expects(:waitpid2).with(pid).returns([pid, 0])

        Puppet::Util.execute('test command')
      end

      it "should close the stdin/stdout/stderr files used by the child" do
        stdin = mock 'file', :close
        stdout = mock 'file', :close
        stderr = mock 'file', :close

        File.expects(:open).
          times(3).
          returns(stdin).
          then.returns(stdout).
          then.returns(stderr)

        Puppet::Util.execute('test command', :squelch => true)
      end

      it "should read and return the output if squelch is false" do
        stdout = Tempfile.new('test')
        Tempfile.stubs(:new).returns(stdout)
        stdout.write("My expected command output")

        Puppet::Util.execute('test command').should == "My expected command output"
      end

      it "should not read the output if squelch is true" do
        stdout = Tempfile.new('test')
        Tempfile.stubs(:new).returns(stdout)
        stdout.write("My expected command output")

        Puppet::Util.execute('test command', :squelch => true).should == nil
      end

      it "should delete the file used for output if squelch is false" do
        stdout = Tempfile.new('test')
        path = stdout.path
        Tempfile.stubs(:new).returns(stdout)

        Puppet::Util.execute('test command')

        File.should_not be_exist(path)
      end

      it "should raise an error if failonfail is true and the child failed" do
        child_status = stub('child_status', :exitstatus => 1)

        Process.expects(:waitpid2).with(pid).returns([pid, child_status])

        expect {
          Puppet::Util.execute('fail command', :failonfail => true)
        }.to raise_error(Puppet::ExecutionFailure, /Execution of 'fail command' returned 1/)
      end

      it "should not raise an error if failonfail is false and the child failed" do
        Process.expects(:waitpid2).with(pid).returns([pid, 1])

        expect {
          Puppet::Util.execute('fail command', :failonfail => false)
        }.not_to raise_error
      end

      it "should not raise an error if failonfail is true and the child succeeded" do
        Process.expects(:waitpid2).with(pid).returns([pid, 0])

        expect {
          Puppet::Util.execute('fail command', :failonfail => true)
        }.not_to raise_error
      end
    end
  end
end
