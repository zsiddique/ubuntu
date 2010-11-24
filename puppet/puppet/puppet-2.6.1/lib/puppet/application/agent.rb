require 'puppet/application'

class Puppet::Application::Agent < Puppet::Application

  should_parse_config
  run_mode :agent

  attr_accessor :args, :agent, :daemon, :host

  def preinit
    # Do an initial trap, so that cancels don't get a stack trace.
    trap(:INT) do
      $stderr.puts "Cancelling startup"
      exit(0)
    end

    {
      :waitforcert => nil,
      :detailed_exitcodes => false,
      :verbose => false,
      :debug => false,
      :centrallogs => false,
      :setdest => false,
      :enable => false,
      :disable => false,
      :client => true,
      :fqdn => nil,
      :serve => [],
      :digest => :MD5,
      :fingerprint => false,
    }.each do |opt,val|
      options[opt] = val
    end

    @args = {}
    require 'puppet/daemon'
    @daemon = Puppet::Daemon.new
    @daemon.argv = ARGV.dup
  end

  option("--centrallogging")
  option("--disable")
  option("--enable")
  option("--debug","-d")
  option("--fqdn FQDN","-f")
  option("--test","-t")
  option("--verbose","-v")

  option("--fingerprint")
  option("--digest DIGEST")

  option("--serve HANDLER", "-s") do |arg|
    if Puppet::Network::Handler.handler(arg)
      options[:serve] << arg.to_sym
    else
      raise "Could not find handler for #{arg}"
    end
  end

  option("--no-client") do |arg|
    options[:client] = false
  end

  option("--detailed-exitcodes") do |arg|
    options[:detailed_exitcodes] = true
  end

  option("--logdest DEST", "-l DEST") do |arg|
    begin
      Puppet::Util::Log.newdestination(arg)
      options[:setdest] = true
    rescue => detail
      puts detail.backtrace if Puppet[:debug]
      $stderr.puts detail.to_s
    end
  end

  option("--waitforcert WAITFORCERT", "-w") do |arg|
    options[:waitforcert] = arg.to_i
  end

  option("--port PORT","-p") do |arg|
    @args[:Port] = arg
  end

  def run_command
    return fingerprint if options[:fingerprint]
    return onetime if Puppet[:onetime]
    main
  end

  def fingerprint
    unless cert = host.certificate || host.certificate_request
      $stderr.puts "Fingerprint asked but no certificate nor certificate request have yet been issued"
      exit(1)
      return
    end
    unless fingerprint = cert.fingerprint(options[:digest])
      raise ArgumentError, "Could not get fingerprint for digest '#{options[:digest]}'"
    end
    Puppet.notice fingerprint
  end

  def onetime
    unless options[:client]
      $stderr.puts "onetime is specified but there is no client"
      exit(43)
      return
    end

    @daemon.set_signal_traps

    begin
      report = @agent.run
    rescue => detail
      puts detail.backtrace if Puppet[:trace]
      Puppet.err detail.to_s
    end

    if not report
      exit(1)
    elsif not Puppet[:noop] and options[:detailed_exitcodes] then
      exit(report.exit_status)
    else
      exit(0)
    end
  end

  def main
    Puppet.notice "Starting Puppet client version #{Puppet.version}"

    @daemon.start
  end

  # Enable all of the most common test options.
  def setup_test
    Puppet.settings.handlearg("--ignorecache")
    Puppet.settings.handlearg("--no-usecacheonfailure")
    Puppet.settings.handlearg("--no-splay")
    Puppet.settings.handlearg("--show_diff")
    Puppet.settings.handlearg("--no-daemonize")
    options[:verbose] = true
    Puppet[:onetime] = true
    options[:detailed_exitcodes] = true
  end

  # Handle the logging settings.
  def setup_logs
    if options[:debug] or options[:verbose]
      Puppet::Util::Log.newdestination(:console)
      if options[:debug]
        Puppet::Util::Log.level = :debug
      else
        Puppet::Util::Log.level = :info
      end
    end

    Puppet::Util::Log.newdestination(:syslog) unless options[:setdest]
  end

  def enable_disable_client(agent)
    if options[:enable]
      agent.enable
    elsif options[:disable]
      agent.disable
    end
    exit(0)
  end

  def setup_listen
    unless FileTest.exists?(Puppet[:authconfig])
      Puppet.err "Will not start without authorization file #{Puppet[:authconfig]}"
      exit(14)
    end

    handlers = nil

    if options[:serve].empty?
      handlers = [:Runner]
    else
      handlers = options[:serve]
    end

    require 'puppet/network/server'
    # No REST handlers yet.
    server = Puppet::Network::Server.new(:xmlrpc_handlers => handlers, :port => Puppet[:puppetport])

    @daemon.server = server
  end

  def setup_host
    @host = Puppet::SSL::Host.new
    waitforcert = options[:waitforcert] || (Puppet[:onetime] ? 0 : 120)
    cert = @host.wait_for_cert(waitforcert) unless options[:fingerprint]
  end

  def setup
    setup_test if options[:test]

    setup_logs

    exit(Puppet.settings.print_configs ? 0 : 1) if Puppet.settings.print_configs?

    # If noop is set, then also enable diffs
    Puppet[:show_diff] = true if Puppet[:noop]

    args[:Server] = Puppet[:server]
    if options[:fqdn]
      args[:FQDN] = options[:fqdn]
      Puppet[:certname] = options[:fqdn]
    end

    if options[:centrallogs]
      logdest = args[:Server]

      logdest += ":" + args[:Port] if args.include?(:Port)
      Puppet::Util::Log.newdestination(logdest)
    end

    Puppet.settings.use :main, :agent, :ssl

    # Always ignoreimport for agent. It really shouldn't even try to import,
    # but this is just a temporary band-aid.
    Puppet[:ignoreimport] = true

    # We need to specify a ca location for all of the SSL-related i
    # indirected classes to work; in fingerprint mode we just need
    # access to the local files and we don't need a ca.
    Puppet::SSL::Host.ca_location = options[:fingerprint] ? :none : :remote

    Puppet::Transaction::Report.terminus_class = :rest

    # Override the default; puppetd needs this, usually.
    # You can still override this on the command-line with, e.g., :compiler.
    Puppet[:catalog_terminus] = :rest

    # Override the default.
    Puppet[:facts_terminus] = :facter

    Puppet::Resource::Catalog.cache_class = :yaml


    # We need tomake the client either way, we just don't start it
    # if --no-client is set.
    require 'puppet/agent'
    require 'puppet/configurer'
    @agent = Puppet::Agent.new(Puppet::Configurer)

    enable_disable_client(@agent) if options[:enable] or options[:disable]

    @daemon.agent = agent if options[:client]

    # It'd be nice to daemonize later, but we have to daemonize before the
    # waitforcert happens.
    @daemon.daemonize if Puppet[:daemonize]

    setup_host

    @objects = []

    # This has to go after the certs are dealt with.
    if Puppet[:listen]
      unless Puppet[:onetime]
        setup_listen
      else
        Puppet.notice "Ignoring --listen on onetime run"
      end
    end
  end
end
