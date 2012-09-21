require 'puppet/application'

class Puppet::Application::Apply < Puppet::Application

  should_parse_config

  option("--debug","-d")
  option("--execute EXECUTE","-e") do |arg|
    options[:code] = arg
  end
  option("--loadclasses","-L")
  option("--verbose","-v")
  option("--use-nodes")
  option("--detailed-exitcodes")

  option("--apply catalog",  "-a catalog") do |arg|
    Puppet.warning <<EOM
--apply is deprecated and will be removed in the future. Please
use 'puppet apply --catalog <catalog>'.
EOM
    options[:catalog] = arg
  end

  option("--catalog catalog",  "-c catalog") do |arg|
    options[:catalog] = arg
  end

  option("--logdest LOGDEST", "-l") do |arg|
    begin
      Puppet::Util::Log.newdestination(arg)
      options[:logset] = true
    rescue => detail
      $stderr.puts detail.to_s
    end
  end

  option("--parseonly") do
    puts "--parseonly has been removed. Please use 'puppet parser validate <manifest>'"
    exit 1
  end

  def help
    <<-HELP

puppet-apply(8) -- Apply Puppet manifests locally
========

SYNOPSIS
--------
Applies a standalone Puppet manifest to the local system.


USAGE
-----
puppet apply [-h|--help] [-V|--version] [-d|--debug] [-v|--verbose]
  [-e|--execute] [--detailed-exitcodes] [-l|--logdest <file>]
  [--apply <catalog>] [--catalog <catalog>] <file>


DESCRIPTION
-----------
This is the standalone puppet execution tool; use it to apply
individual manifests.

When provided with a modulepath, via command line or config file, puppet
apply can effectively mimic the catalog that would be served by puppet
master with access to the same modules, although there are some subtle
differences. When combined with scheduling and an automated system for
pushing manifests, this can be used to implement a serverless Puppet
site.

Most users should use 'puppet agent' and 'puppet master' for site-wide
manifests.


OPTIONS
-------
Note that any configuration parameter that's valid in the configuration
file is also a valid long argument. For example, 'modulepath' is a
valid configuration parameter, so you can specify '--tags <class>,<tag>'
as an argument.

See the configuration file documentation at
http://docs.puppetlabs.com/references/stable/configuration.html for the
full list of acceptable parameters. A commented list of all
configuration options can also be generated by running puppet with
'--genconfig'.

* --debug:
  Enable full debugging.

* --detailed-exitcodes:
  Provide transaction information via exit codes. If this is enabled, an exit
  code of '2' means there were changes, an exit code of '4' means there were
  failures during the transaction, and an exit code of '6' means there were both
  changes and failures.

* --help:
  Print this help message

* --loadclasses:
  Load any stored classes. 'puppet agent' caches configured classes
  (usually at /etc/puppet/classes.txt), and setting this option causes
  all of those classes to be set in your puppet manifest.

* --logdest:
  Where to send messages. Choose between syslog, the console, and a log
  file. Defaults to sending messages to the console.

* --execute:
  Execute a specific piece of Puppet code

* --verbose:
  Print extra information.

* --apply:
  Apply a JSON catalog (such as one generated with 'puppet master --compile'). You can
  either specify a JSON file or pipe in JSON from standard input. Deprecated, please
  use --catalog instead.

* --catalog:
  Apply a JSON catalog (such as one generated with 'puppet master --compile'). You can
  either specify a JSON file or pipe in JSON from standard input.


EXAMPLE
-------
    $ puppet apply -l /tmp/manifest.log manifest.pp
    $ puppet apply --modulepath=/root/dev/modules -e "include ntpd::server"
    $ puppet apply --catalog catalog.json


AUTHOR
------
Luke Kanies


COPYRIGHT
---------
Copyright (c) 2011 Puppet Labs, LLC Licensed under the Apache 2.0 License

    HELP
  end

  def run_command
    if options[:catalog]
      apply
    else
      main
    end
  end

  def apply
    if options[:catalog] == "-"
      text = $stdin.read
    else
      text = ::File.read(options[:catalog])
    end

    begin
      catalog = Puppet::Resource::Catalog.convert_from(Puppet::Resource::Catalog.default_format,text)
      catalog = Puppet::Resource::Catalog.pson_create(catalog) unless catalog.is_a?(Puppet::Resource::Catalog)
    rescue => detail
      raise Puppet::Error, "Could not deserialize catalog from pson: #{detail}"
    end

    catalog = catalog.to_ral

    require 'puppet/configurer'
    configurer = Puppet::Configurer.new
    configurer.run :catalog => catalog
  end

  def main
    # Set our code or file to use.
    if options[:code] or command_line.args.length == 0
      Puppet[:code] = options[:code] || STDIN.read
    else
      manifest = command_line.args.shift
      raise "Could not find file #{manifest}" unless ::File.exist?(manifest)
      Puppet.warning("Only one file can be applied per run.  Skipping #{command_line.args.join(', ')}") if command_line.args.size > 0
      Puppet[:manifest] = manifest
    end

    unless Puppet[:node_name_fact].empty?
      # Collect our facts.
      unless facts = Puppet::Node::Facts.indirection.find(Puppet[:node_name_value])
        raise "Could not find facts for #{Puppet[:node_name_value]}"
      end

      Puppet[:node_name_value] = facts.values[Puppet[:node_name_fact]]
      facts.name = Puppet[:node_name_value]
    end

    # Find our Node
    unless node = Puppet::Node.indirection.find(Puppet[:node_name_value])
      raise "Could not find node #{Puppet[:node_name_value]}"
    end

    # Merge in the facts.
    node.merge(facts.values) if facts

    # Allow users to load the classes that puppet agent creates.
    if options[:loadclasses]
      file = Puppet[:classfile]
      if FileTest.exists?(file)
        unless FileTest.readable?(file)
          $stderr.puts "#{file} is not readable"
          exit(63)
        end
        node.classes = ::File.read(file).split(/[\s\n]+/)
      end
    end

    begin
      # Compile our catalog
      starttime = Time.now
      catalog = Puppet::Resource::Catalog.indirection.find(node.name, :use_node => node)

      # Translate it to a RAL catalog
      catalog = catalog.to_ral

      catalog.finalize

      catalog.retrieval_duration = Time.now - starttime

      require 'puppet/configurer'
      configurer = Puppet::Configurer.new
      report = configurer.run(:skip_plugin_download => true, :catalog => catalog)

      if not report
        exit(1)
      elsif options[:detailed_exitcodes] then
        exit(report.exit_status)
      else
        exit(0)
      end
    rescue => detail
      puts detail.backtrace if Puppet[:trace]
      $stderr.puts detail.message
      exit(1)
    end
  end

  def setup
    exit(Puppet.settings.print_configs ? 0 : 1) if Puppet.settings.print_configs?

    Puppet::Util::Log.newdestination(:console) unless options[:logset]
    client = nil
    server = nil

    Signal.trap(:INT) do
      $stderr.puts "Exiting"
      exit(1)
    end

    # we want the last report to be persisted locally
    Puppet::Transaction::Report.indirection.cache_class = :yaml

    if options[:debug]
      Puppet::Util::Log.level = :debug
    elsif options[:verbose]
      Puppet::Util::Log.level = :info
    end
  end
end