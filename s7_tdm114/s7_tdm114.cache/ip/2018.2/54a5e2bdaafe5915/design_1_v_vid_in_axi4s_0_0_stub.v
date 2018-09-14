// Copyright 1986-2018 Xilinx, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2018.2 (win64) Build 2258646 Thu Jun 14 20:03:12 MDT 2018
// Date        : Sun Sep  9 16:52:23 2018
// Host        : DESKTOP-HQKVQ13 running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub -rename_top decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix -prefix
//               decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix_ design_1_v_vid_in_axi4s_0_0_stub.v
// Design      : design_1_v_vid_in_axi4s_0_0
// Purpose     : Stub declaration of top-level module interface
// Device      : xc7s50csga324-1
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* X_CORE_INFO = "v_vid_in_axi4s_v4_0_8,Vivado 2018.2" *)
module decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix(vid_io_in_ce, vid_active_video, vid_vblank, 
  vid_hblank, vid_vsync, vid_hsync, vid_field_id, vid_data, aclk, aclken, aresetn, 
  m_axis_video_tdata, m_axis_video_tvalid, m_axis_video_tready, m_axis_video_tuser, 
  m_axis_video_tlast, fid, vtd_active_video, vtd_vblank, vtd_hblank, vtd_vsync, vtd_hsync, 
  vtd_field_id, overflow, underflow, axis_enable)
/* synthesis syn_black_box black_box_pad_pin="vid_io_in_ce,vid_active_video,vid_vblank,vid_hblank,vid_vsync,vid_hsync,vid_field_id,vid_data[7:0],aclk,aclken,aresetn,m_axis_video_tdata[7:0],m_axis_video_tvalid,m_axis_video_tready,m_axis_video_tuser,m_axis_video_tlast,fid,vtd_active_video,vtd_vblank,vtd_hblank,vtd_vsync,vtd_hsync,vtd_field_id,overflow,underflow,axis_enable" */;
  input vid_io_in_ce;
  input vid_active_video;
  input vid_vblank;
  input vid_hblank;
  input vid_vsync;
  input vid_hsync;
  input vid_field_id;
  input [7:0]vid_data;
  input aclk;
  input aclken;
  input aresetn;
  output [7:0]m_axis_video_tdata;
  output m_axis_video_tvalid;
  input m_axis_video_tready;
  output m_axis_video_tuser;
  output m_axis_video_tlast;
  output fid;
  output vtd_active_video;
  output vtd_vblank;
  output vtd_hblank;
  output vtd_vsync;
  output vtd_hsync;
  output vtd_field_id;
  output overflow;
  output underflow;
  input axis_enable;
endmodule
