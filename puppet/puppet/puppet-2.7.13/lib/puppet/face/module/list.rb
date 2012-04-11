Puppet::Face.define(:module, '1.0.0') do
  action(:list) do
    summary "List installed modules"
    description <<-HEREDOC
      List puppet modules from a specific environment, specified modulepath or
      default to listing modules in the default modulepath.  The output will
      include information about unmet module dependencies based on information
      from module metadata.
      #{Puppet.settings[:modulepath]}
    HEREDOC
    returns "hash of paths to module objects"

    option "--env ENVIRONMENT" do
      summary "Which environments' modules to list"
    end

    option "--modulepath MODULEPATH" do
      summary "Which directories to look for modules in"
    end

    examples <<-EOT
      List installed modules:

      $ puppet module list
        /etc/puppet/modules
          bacula (0.0.2)
        /usr/share/puppet/modules
          apache (0.0.3)
          bacula (0.0.1)

      List installed modules from a specified environment:

      $ puppet module list --env 'test'
        Missing dependency `stdlib`:
          `rrd` (0.0.2) requires `puppetlabs/stdlib` (>= 2.2.0)

        /tmp/puppet/modules
          rrd (0.0.2)

      List installed modules from a specified modulepath:

      $ puppet module list --modulepath /tmp/facts1:/tmp/facts2
        /tmp/facts1
          stdlib
        /tmp/facts2
          nginx (1.0.0)
    EOT

    when_invoked do |options|
      Puppet[:modulepath] = options[:modulepath] if options[:modulepath]
      environment = Puppet::Node::Environment.new(options[:env])

      environment.modules_by_path
    end

    when_rendering :console do |modules_by_path, options|
      output = ''

      Puppet[:modulepath] = options[:modulepath] if options[:modulepath]
      environment = Puppet::Node::Environment.new(options[:env])

      dependency_errors = false

      environment.modules.sort_by {|mod| mod.name}.each do |mod|
        mod.unmet_dependencies.sort_by {|dep| dep[:name]}.each do |dep|
          dependency_errors = true
          $stderr.puts dep[:error]
        end
      end

      output << "\n" if dependency_errors

      modules_by_path.each do |path, modules|
        output << "#{path}\n"
        modules.sort_by {|mod| mod.name }.each do |mod|
          version_string = mod.version ? "(#{mod.version})" : ''
          output << "  #{mod.name} #{version_string}\n"
        end
      end
      output
    end

  end
end
