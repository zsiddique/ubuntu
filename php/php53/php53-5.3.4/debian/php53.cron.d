# /etc/cron.d/php5: crontab fragment for php5
#  This purges session files older than X, where X is defined in seconds
#  as the largest value of session.gc_maxlifetime from all your php.ini
#  files, or 24 minutes if not defined.  See /usr/lib/php5/maxlifetime

# Look for and purge old sessions every 30 minutes
09,39 *     * * *     root   [ -x /usr/lib/php53/maxlifetime ] && [ -d /var/lib/php53 ] && find /var/lib/php53/ -type f -cmin +$(/usr/lib/php53/maxlifetime) -print0 | xargs -r -0 rm