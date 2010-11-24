#!/usr/bin/env ruby

require File.dirname(__FILE__) + '/../../spec_helper'

module ScheduleTesting

  def format(time)
    time.strftime("%H:%M:%S")
  end

  def diff(unit, incr, method, count)
    diff = Time.now.to_i.send(method, incr * count)
    Time.at(diff)
  end

  def month(method, count)
    diff(:hour, 3600 * 24 * 30, method, count)
  end

  def week(method, count)
    diff(:hour, 3600 * 24 * 7, method, count)
  end

  def day(method, count)
    diff(:hour, 3600 * 24, method, count)
  end

  def hour(method, count)
    diff(:hour, 3600, method, count)
  end

  def min(method, count)
    diff(:min, 60, method, count)
  end

  def sec(method, count)
    diff(:sec, 1, method, count)
  end

end

describe Puppet::Type.type(:schedule) do
  before :each do
    Puppet[:ignoreschedules] = false

    @schedule = Puppet::Type.type(:schedule).new(:name => "testing")
  end

  describe Puppet::Type.type(:schedule) do
    include ScheduleTesting

    it "should default to :distance for period-matching" do
      @schedule[:periodmatch].should == :distance
    end

    it "should default to a :repeat of 1" do
      @schedule[:repeat].should == 1
    end

    it "should never match when the period is :never" do
      @schedule[:period] = :never
      @schedule.match?.should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when producing default schedules" do
    include ScheduleTesting

    %w{hourly daily weekly monthly never}.each do |period|
      period = period.to_sym
      it "should produce a #{period} schedule with the period set appropriately" do
        schedules = Puppet::Type.type(:schedule).mkdefaultschedules
        schedules.find { |s| s[:name] == period.to_s and s[:period] == period }.should be_instance_of(Puppet::Type.type(:schedule))
      end
    end

    it "should produce a schedule named puppet with a period of hourly and a repeat of 2" do
      schedules = Puppet::Type.type(:schedule).mkdefaultschedules
      schedules.find { |s|
        s[:name] == "puppet" and s[:period] == :hourly and s[:repeat] == 2
      }.should be_instance_of(Puppet::Type.type(:schedule))
    end
  end

  describe Puppet::Type.type(:schedule), "when matching ranges" do
    include ScheduleTesting

    it "should match when the start time is before the current time and the end time is after the current time" do
      @schedule[:range] = "#{format(Time.now - 10)} - #{format(Time.now + 10)}"
      @schedule.match?.should be_true
    end

    it "should not match when the start time is after the current time" do
      @schedule[:range] = "#{format(Time.now + 5)} - #{format(Time.now + 10)}"
      @schedule.match?.should be_false
    end

    it "should not match when the end time is previous to the current time" do
      @schedule[:range] = "#{format(Time.now - 10)} - #{format(Time.now - 5)}"
      @schedule.match?.should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching hourly by distance" do
    include ScheduleTesting

    before do
      @schedule[:period] = :hourly
      @schedule[:periodmatch] = :distance
    end

    it "should match an hour ago" do
      @schedule.match?(hour("-", 1)).should be_true
    end

    it "should not match now" do
      @schedule.match?(Time.now).should be_false
    end

    it "should not match 59 minutes ago" do
      @schedule.match?(min("-", 59)).should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching daily by distance" do
    include ScheduleTesting

    before do
      @schedule[:period] = :daily
      @schedule[:periodmatch] = :distance
    end

    it "should match when the previous time was one day ago" do
      @schedule.match?(day("-", 1)).should be_true
    end

    it "should not match when the previous time is now" do
      @schedule.match?(Time.now).should be_false
    end

    it "should not match when the previous time was 23 hours ago" do
      @schedule.match?(hour("-", 23)).should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching weekly by distance" do
    include ScheduleTesting

    before do
      @schedule[:period] = :weekly
      @schedule[:periodmatch] = :distance
    end

    it "should match seven days ago" do
      @schedule.match?(day("-", 7)).should be_true
    end

    it "should not match now" do
      @schedule.match?(Time.now).should be_false
    end

    it "should not match six days ago" do
      @schedule.match?(day("-", 6)).should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching monthly by distance" do
    include ScheduleTesting

    before do
      @schedule[:period] = :monthly
      @schedule[:periodmatch] = :distance
    end

    it "should match 32 days ago" do
      @schedule.match?(day("-", 32)).should be_true
    end

    it "should not match now" do
      @schedule.match?(Time.now).should be_false
    end

    it "should not match 27 days ago" do
      @schedule.match?(day("-", 27)).should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching hourly by number" do
    include ScheduleTesting

    before do
      @schedule[:period] = :hourly
      @schedule[:periodmatch] = :number
    end

    it "should match if the times are one minute apart and the current minute is 0" do
      current = Time.now

      # Subtract an hour, reset the minute to zero, then add 59 minutes, so we're the previous hour plus 59 minutes.
      previous = (current - 3600 - (current.min * 60) + (59 * 60))

      # Now set the "current" time to the zero minute of the current hour.
      now = (current - (current.min * 60))
      Time.stubs(:now).returns(now)
      @schedule.match?(previous).should be_true
    end

    it "should not match if the times are 58 minutes apart and the current minute is 59" do
      current = Time.now

      # reset the minute to zero
      previous = current - (current.min * 60)

      # Now set the "current" time to the 59th minute of the current hour.
      now = (current - (current.min * 60) + (59 * 60))
      Time.stubs(:now).returns(now)
      @schedule.match?(previous).should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching daily by number" do
    include ScheduleTesting

    before do
      @schedule[:period] = :daily
      @schedule[:periodmatch] = :number
    end

    it "should match if the times are one minute apart and the current minute and hour are 0" do
      zero = Time.now

      # Reset the current time to X:00:00
      current = zero - (zero.hour * 3600) - (zero.min * 60) - zero.sec

      # Now set the previous time to one minute before that
      previous = current - 60

      Time.stubs(:now).returns(current)
      @schedule.match?(previous).should be_true
    end

    it "should not match if the times are 23 hours and 58 minutes apart and the current hour is 23 and the current minute is 59" do
      zero = Time.now

      # Reset the previous time to 00:00:00
      previous = zero - (zero.hour * 3600) - (zero.min * 60) - zero.sec

      # Set the current time to 23:59
      now = previous + (23 * 3600) + (59 * 60)

      Time.stubs(:now).returns(now)
      @schedule.match?(previous).should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching weekly by number" do
    include ScheduleTesting

    before do
      @schedule[:period] = :weekly
      @schedule[:periodmatch] = :number
    end

    it "should match if the previous time is prior to the most recent Sunday" do
      now = Time.now

      # Subtract the number days we've progressed into the week, plus one because we're zero-indexed.
      previous = now - (3600 * 24 * (now.wday + 1))

      @schedule.match?(previous).should be_true
    end

    it "should not match if the previous time is after the most recent Saturday" do
      now = Time.now

      # Subtract the number days we've progressed into the week
      previous = now - (3600 * 24 * now.wday)

      @schedule.match?(previous).should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching monthly by number" do
    include ScheduleTesting

    before do
      @schedule[:period] = :monthly
      @schedule[:periodmatch] = :number
    end

    it "should match when the previous time is prior to the first day of this month" do
      now = Time.now

      # Subtract the number days we've progressed into the month
      previous = now - (3600 * 24 * now.day)

      @schedule.match?(previous).should be_true
    end

    it "should not match when the previous time is after the last day of last month" do
      now = Time.now

      # Subtract the number days we've progressed into the month, minus one
      previous = now - (3600 * 24 * (now.day - 1))

      @schedule.match?(previous).should be_false
    end
  end

  describe Puppet::Type.type(:schedule), "when matching with a repeat greater than one" do
    include ScheduleTesting

    before do
      @schedule[:period] = :daily
      @schedule[:repeat] = 2
    end

    it "should fail if the periodmatch is 'number'" do
      @schedule[:periodmatch] = :number
      proc { @schedule[:repeat] = 2 }.should raise_error(Puppet::Error)
    end

    it "should match if the previous run was further away than the distance divided by the repeat" do
      previous = Time.now - (3600 * 13)
      @schedule.match?(previous).should be_true
    end

    it "should not match if the previous run was closer than the distance divided by the repeat" do
      previous = Time.now - (3600 * 11)
      @schedule.match?(previous).should be_false
    end
  end
end
