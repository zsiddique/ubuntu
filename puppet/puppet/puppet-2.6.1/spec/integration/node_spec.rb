#!/usr/bin/env ruby
#
#  Created by Luke Kanies on 2007-9-23.
#  Copyright (c) 2007. All rights reserved.

require File.dirname(__FILE__) + '/../spec_helper'

require 'puppet/node'

describe Puppet::Node do
  describe "when delegating indirection calls" do
    before do
      @name = "me"
      @node = Puppet::Node.new(@name)
    end

    it "should be able to use the exec terminus" do
      Puppet::Node.indirection.stubs(:terminus_class).returns :exec

      # Load now so we can stub
      terminus = Puppet::Node.indirection.terminus(:exec)

      terminus.expects(:query).with(@name).returns "myresults"
      terminus.expects(:translate).with(@name, "myresults").returns "translated_results"
      terminus.expects(:create_node).with(@name, "translated_results").returns @node

      Puppet::Node.find(@name).should equal(@node)
    end

    it "should be able to use the yaml terminus" do
      Puppet::Node.indirection.stubs(:terminus_class).returns :yaml

      # Load now, before we stub the exists? method.
      terminus = Puppet::Node.indirection.terminus(:yaml)

      terminus.expects(:path).with(@name).returns "/my/yaml/file"

      FileTest.expects(:exist?).with("/my/yaml/file").returns false
      Puppet::Node.find(@name).should be_nil
    end

    it "should have an ldap terminus" do
      Puppet::Node.indirection.terminus(:ldap).should_not be_nil
    end

    it "should be able to use the plain terminus" do
      Puppet::Node.indirection.stubs(:terminus_class).returns :plain

      # Load now, before we stub the exists? method.
      Puppet::Node.indirection.terminus(:plain)

      Puppet::Node.expects(:new).with(@name).returns @node

      Puppet::Node.find(@name).should equal(@node)
    end

    describe "and using the memory terminus" do
      before do
        @name = "me"
        @old_terminus = Puppet::Node.indirection.terminus_class
        @terminus = Puppet::Node.indirection.terminus(:memory)
        Puppet::Node.indirection.stubs(:terminus).returns @terminus
        @node = Puppet::Node.new(@name)
      end

      it "should find no nodes by default" do
        Puppet::Node.find(@name).should be_nil
      end

      it "should be able to find nodes that were previously saved" do
        @node.save
        Puppet::Node.find(@name).should equal(@node)
      end

      it "should replace existing saved nodes when a new node with the same name is saved" do
        @node.save
        two = Puppet::Node.new(@name)
        two.save
        Puppet::Node.find(@name).should equal(two)
      end

      it "should be able to remove previously saved nodes" do
        @node.save
        Puppet::Node.destroy(@node.name)
        Puppet::Node.find(@name).should be_nil
      end

      it "should fail when asked to destroy a node that does not exist" do
        proc { Puppet::Node.destroy(@node) }.should raise_error(ArgumentError)
      end
    end
  end
end
