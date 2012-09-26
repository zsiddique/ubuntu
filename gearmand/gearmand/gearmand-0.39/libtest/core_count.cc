/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Data Differential YATL (i.e. libtest)  library
 *
 *  Copyright (C) 2012 Data Differential, http://datadifferential.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <config.h>

#include <cstdlib>
#include <fcntl.h>
#include <getopt.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libtest/cpu.hpp>

static void version_command(const char *command_name, int major_version, int minor_version)
{
  std::cout << command_name << " " << major_version << "." << minor_version << std::endl;
}

static void help_command(const char *command_name,
                         int major_version, int minor_version,
                         const struct option *long_options)
{
  std::cout << command_name << " " << major_version << "." << minor_version << std::endl;
  std::cout << "Prints the number of cores found on the local host." << std::endl << std::endl;

  for (uint32_t x= 0; long_options[x].name; x++)
  {
    std::cout << "\t --" << long_options[x].name << char(long_options[x].has_arg ? '=' : ' ') << std::endl;
  }

  std::cout << std::endl;
}

enum {
  OPT_HELP,
  OPT_VERSION
};

static void options_parse(int argc, char *argv[])
{
  static struct option long_options[]=
  {
    { "version", no_argument, NULL, OPT_VERSION},
    { "help", no_argument, NULL, OPT_HELP},
    {0, 0, 0, 0},
  };

  bool opt_version= false;
  bool opt_help= false;
  int option_index= 0;

  while (1)
  {
    int option_rv= getopt_long(argc, argv, "", long_options, &option_index);
    if (option_rv == -1) 
    {
      break;
    }

    switch (option_rv)
    {
    case OPT_HELP: /* --help or -h */
      opt_help= true;
      break;

    case OPT_VERSION: /* --version or -v */
      opt_version= true;
      break;

    case '?':
      /* getopt_long already printed an error message. */
      exit(EXIT_FAILURE);

    default:
      help_command(argv[0], 1, 0, long_options);
      exit(EXIT_FAILURE);
    }
  }

  if (opt_version)
  {
    version_command(argv[0], 1, 0);
    exit(EXIT_SUCCESS);
  }

  if (opt_help)
  {
    help_command(argv[0], 1, 0, long_options);
    exit(EXIT_SUCCESS);
  }
}

int main(int argc, char *argv[])
{
  options_parse(argc, argv);

  std::cout << libtest::number_of_cpus() << std::endl;

  return EXIT_SUCCESS;
}
