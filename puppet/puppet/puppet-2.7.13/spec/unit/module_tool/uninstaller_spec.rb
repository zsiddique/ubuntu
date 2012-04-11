require 'spec_helper'
require 'puppet/module_tool'
require 'tmpdir'

describe Puppet::Module::Tool::Applications::Uninstaller do
  include PuppetSpec::Files

  def mkmod(name, path, metadata=nil)
    modpath = File.join(path, name)
    FileUtils.mkdir_p(modpath)

    # For some tests we need the metadata to be present, mainly
    # when testing against specific versions of a module.
    if metadata
      File.open(File.join(modpath, 'metadata.json'), 'w') do |f|
        f.write(metadata.to_pson)
      end
    end

    modpath
  end

  describe "the behavior of the instances" do

    before do
      @uninstaller = Puppet::Module::Tool::Applications::Uninstaller
      FileUtils.mkdir_p(modpath1)
      FileUtils.mkdir_p(modpath2)
      fake_env.modulepath = [modpath1, modpath2]
    end

    let(:modpath1) { File.join(tmpdir("uninstaller"), "modpath1") }
    let(:modpath2) { File.join(tmpdir("uninstaller"), "modpath2") }
    let(:fake_env) { Puppet::Node::Environment.new('fake_env') }
    let(:options)  { {:environment => "fake_env"} }

    context "when the module is not installed" do
      it "should return an empty list" do
        results = @uninstaller.new('fakemod_not_installed', options).run
        results[:removed_mods].should == []
      end
    end

    context "when the module is installed" do
      it "should uninstall the module" do
        foo = mkmod("foo", modpath1)

        results = @uninstaller.new("foo", options).run
        results[:removed_mods].should == [
          Puppet::Module.new('foo', :environment => fake_env, :path => foo)
        ]
      end

      it "should only uninstall the requested module" do
        foo = mkmod("foo", modpath1)

        results = @uninstaller.new("foo", options).run
        results[:removed_mods].should == [
          Puppet::Module.new("foo", :environment => fake_env, :path => foo)
        ]
      end

      it "should uninstall the module from every path in the modpath" do
        foo1 = mkmod('foo', modpath1)
        foo2 = mkmod('foo', modpath2)

        results = @uninstaller.new('foo', options).run
        results[:removed_mods].length.should == 2
        results[:removed_mods].should include(
          Puppet::Module.new('foo', :environment => fake_env, :path => foo1),
          Puppet::Module.new('foo', :environment => fake_env, :path => foo2)
        )
      end

      context "when options[:version] is specified" do
        let(:metadata) do
          {
            "author"       => "",
            "name"         => "foo",
            "version"      => "1.0.0",
            "source"       => "http://dummyurl",
            "license"      => "Apache2",
            "dependencies" => [],
          }
        end

        it "should uninstall the module if the version matches" do
          foo = mkmod('foo', modpath1, metadata)

          options[:version] = "1.0.0"

          results = @uninstaller.new("foo", options).run
          results[:removed_mods].length.should == 1
          results[:removed_mods].first.name.should == "foo"
          results[:removed_mods].first.version.should == "1.0.0"
        end

        it "should not uninstall the module if the version does not match" do
          foo = mkmod("foo", modpath1, metadata)

          options[:version] = "2.0.0"

          results = @uninstaller.new("foo", options).run
          results[:removed_mods].should == []
        end

        context "when the module metadata is missing" do
          it "should not uninstall the module" do
            foo = mkmod("foo", modpath1)

            options[:version] = "2.0.0"

            results = @uninstaller.new("foo", options).run
            results[:removed_mods].should == []
          end
        end
      end

      # This test is pending work in #11803 to which will add
      # dependency resolution.
      it "should check for broken dependencies"
    end
  end
end
