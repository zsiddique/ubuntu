#!/usr/bin/env ruby

require File.dirname(__FILE__) + '/../../../spec_helper'

provider_class = Puppet::Type.type(:user).provider(:useradd)

describe provider_class do
  before do
    @resource = stub("resource", :name => "myuser", :managehome? => nil)
    @resource.stubs(:should).returns "fakeval"
    @resource.stubs(:[]).returns "fakeval"
    @provider = provider_class.new(@resource)
  end

  # #1360
  it "should add -o when allowdupe is enabled and the user is being created" do
    @resource.expects(:allowdupe?).returns true
    @provider.expects(:execute).with { |args| args.include?("-o") }
    @provider.create
  end

  it "should add -o when allowdupe is enabled and the uid is being modified" do
    @resource.expects(:allowdupe?).returns true
    @provider.expects(:execute).with { |args| args.include?("-o") }

    @provider.uid = 150
  end

  describe "when checking to add allow dup" do
    it "should check allow dup" do
      @resource.expects(:allowdupe?)
      @provider.check_allow_dup
    end

    it "should return an array with a flag if dup is allowed" do
      @resource.stubs(:allowdupe?).returns true
      @provider.check_allow_dup.must == ["-o"]
    end

    it "should return an empty array if no dup is allowed" do
      @resource.stubs(:allowdupe?).returns false
      @provider.check_allow_dup.must == []
    end
  end

  describe "when checking manage home" do
    it "should check manage home" do
      @resource.expects(:managehome?)
      @provider.check_manage_home
    end

    it "should return an array with -m flag if home is managed" do
      @resource.stubs(:managehome?).returns true
      @provider.check_manage_home.must == ["-m"]
    end

    it "should return an array with -M if home is not managed and on Redhat" do
      Facter.stubs(:value).with("operatingsystem").returns("RedHat")
      @resource.stubs(:managehome?).returns false
      @provider.check_manage_home.must == ["-M"]
    end

    it "should return an empty array if home is not managed and not on Redhat" do
      Facter.stubs(:value).with("operatingsystem").returns("some OS")
      @resource.stubs(:managehome?).returns false
      @provider.check_manage_home.must == []
    end
  end

  describe "when adding properties" do
    it "should get the valid properties"
    it "should not add the ensure property"
    it "should add the flag and value to an array"
    it "should return and array of flags and values"
  end

  describe "when calling addcmd" do
    before do
      @resource.stubs(:allowdupe?).returns true
      @resource.stubs(:managehome?).returns true
    end

    it "should call command with :add" do
      @provider.expects(:command).with(:add)
      @provider.addcmd
    end

    it "should add properties" do
      @provider.expects(:add_properties).returns([])
      @provider.addcmd
    end

    it "should check and add if dup allowed" do
      @provider.expects(:check_allow_dup).returns([])
      @provider.addcmd
    end

    it "should check and add if home is managed" do
      @provider.expects(:check_manage_home).returns([])
      @provider.addcmd
    end

    it "should add the resource :name" do
      @resource.expects(:[]).with(:name)
      @provider.addcmd
    end

    it "should return an array with full command" do
      @provider.stubs(:command).with(:add).returns("useradd")
      @provider.stubs(:add_properties).returns(["-G", "somegroup"])
      @resource.stubs(:[]).with(:name).returns("someuser")
      @provider.addcmd.must == ["useradd", "-G", "somegroup", "-o", "-m", "someuser"]
    end
  end
end
