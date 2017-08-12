#!/usr/bin/env ruby
require "RubyROOT"
require "RubyFits"
include Root
include Fits
include Math

def plotSetting(hist, xtitle, ytitle)
  hist.SetTitle("")
  hist.GetXaxis().SetTitle(xtitle)
  hist.GetYaxis().SetTitle(ytitle)
  hist.SetStats(0)
  hist.Sumw2()
end

if ARGV[3]==nil then
  puts "Usage: ruby norikura_ql.rb [ql folder] [list file] [detector name] [lc bin width] [channel number] [threshold 0ch] .."
  exit -1
end
  
fitsList=ARGV[1]
detector=ARGV[2]
lc_width=ARGV[3].to_f
num_chan=ARGV[4].to_i
#ql_dir="/home/growth/work/ql"
ql_dir=ARGV[0]

if File.exists?("#{ql_dir}/#{detector}")==false then
  `mkdir #{ql_dir}/#{detector}`
end

c0=Root::TCanvas.create("c0", "canvas0", 640, 480)
File.open(fitsList, "r") do |list|
  list.each_line do |fitsAddress|
    fitsAddress.chomp!
    fitsAddressParse=fitsAddress.split("/")
    fitsName=fitsAddressParse[fitsAddressParse.length-1]
    fitsNameParse=fitsName.split(".")
    fitsHeader=fitsNameParse[0]

    puts fitsAddress
    
    fits=Fits::FitsFile.new(fitsAddress)
    eventHDU=fits["EVENTS"]
    timeTag=eventHDU["timeTag"]
    phaMax=eventHDU["phaMax"]
    phaMin=eventHDU["phaMin"]
    triggerCount=eventHDU["triggerCount"]
    adcIndex=eventHDU["boardIndexAndChannel"]
    eventNum=eventHDU.getNRows()-1

    startTimeTag=timeTag[0].to_i
    stopTimeTag=timeTag[eventNum].to_i

    if stopTimeTag<startTimeTag then
      observationTime=(stopTimeTag-startTimeTag+2**40).to_f/1.0e8
    else
      observationTime=(stopTimeTag-startTimeTag).to_f/1.0e8
    end

    puts observationTime
    
    spec_binNum=2048
    lc_binNum=(observationTime/lc_width).to_i+1
    lc_binEnd=lc_width*lc_binNum.to_f
    spec=Root::TH1F.create("spec", "spec", spec_binNum, 2046.5, 4095.5)
    lc_all=Root::TH1F.create("lc_all", "lc_all", lc_binNum, 0.0, lc_binEnd)
    lc_high=Root::TH1F.create("lc_high", "lc_high", lc_binNum, 0.0, lc_binEnd)
    waveform=Root::TH1F.create("waveform", "waveform", lc_binNum, 0.0, lc_binEnd)
    delta_trigger=Root::TH1F.create("delta_trigger", "delta_trigger", lc_binNum, 0.0, lc_binEnd)

    eventTimeArray=Array.new
    phaMaxArray=Array.new
    phaMinArray=Array.new
    
    for adc in 0..num_chan
      spec.Reset()
      lc_all.Reset()
      lc_high.Reset()
      threshold=ARGV[adc+5].to_i
      index=0
      pastCount=-1.0
      for i in 0..eventNum
        if adcIndex[i].to_i==adc then
          spec.Fill(phaMax[i].to_f)
          if startTimeTag<=timeTag[i].to_i then
            eventTime=(timeTag[i].to_i-startTimeTag).to_f/1.0e8
          else
            eventTime=(timeTag[i].to_i-startTimeTag+2**40).to_f/1.0e8
          end
          eventTimeArray[index]=eventTime
          phaMaxArray[index]=phaMax[i].to_f
          phaMinArray[index]=phaMin[i].to_f
          lc_all.Fill(eventTime)
          if phaMax[i].to_i>=threshold
            lc_high.Fill(eventTime)
          end
          index+=1
          if pastCount<0 then
            pastCount=triggerCount[i].to_i
          else
            deltaCount=triggerCount[i].to_i-pastCount-1
            if deltaCount<0 then
              deltaCount+=2**16
            end
            deltaCount.times do
              delta_trigger.Fill(eventTime)
            end
            pastCount=triggerCount[i].to_i
          end
        end
      end
      phaMaxWaveform=Root::TGraph.create(eventTimeArray, phaMaxArray)
      phaMinWaveform=Root::TGraph.create(eventTimeArray, phaMinArray)
      spec.Scale(1.0/observationTime)
      lc_all.Scale(1.0/lc_width)
      lc_high.Scale(1.0/lc_width)
      plotSetting(spec, "channel", "Count s^{-1} ch^{-1}")
      plotSetting(lc_all, "Second", "Count s^{-1}")
      plotSetting(lc_high, "Second", "Count s^{-1}")
      plotSetting(waveform, "Second", "ADC Channel")
      plotSetting(delta_trigger, "Second", "unread event")

      waveform.GetYaxis().SetRangeUser(0, 4096)
      phaMaxWaveform.SetMarkerStyle(7)
      phaMinWaveform.SetMarkerStyle(7)
      phaMaxWaveform.SetMarkerColor(1)
      phaMinWaveform.SetMarkerColor(2)
      
      spec.Draw("h")
      c0.SetLogy(1)
      c0.Update()
      c0.SaveAs("#{ql_dir}/#{detector}/#{fitsHeader}_ch#{adc.to_s}_spec.pdf")
      lc_all.Draw("h")
      c0.SetLogy(0)
      c0.Update()
      c0.SaveAs("#{ql_dir}/#{detector}/#{fitsHeader}_ch#{adc.to_s}_lc_all.pdf")
      lc_high.Draw("h")
      c0.Update()
      c0.SaveAs("#{ql_dir}/#{detector}/#{fitsHeader}_ch#{adc.to_s}_lc_high.pdf")
      waveform.Draw("h")
      phaMaxWaveform.Draw("samep")
      phaMinWaveform.Draw("samep")
      c0.Update()
      c0.SaveAs("#{ql_dir}/#{detector}/#{fitsHeader}_ch#{adc.to_s}_waveform.png")
      delta_trigger.Draw("h")
      c0.Update()
      c0.SaveAs("#{ql_dir}/#{detector}/#{fitsHeader}_ch#{adc.to_s}_delta_trigger.pdf")
    end
  end
end
