##############################################################################
## This file is part of 'SLAC Firmware Standard Library'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'SLAC Firmware Standard Library', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

## \file vivado_hls_interactive.tcl
# \brief This script launches the Vivado HLS interface TCL mode with all the 
# ruckus procedures and environmental variables included 

## Get variables and Custom Procedures
set RUCKUS_DIR $::env(RUCKUS_DIR)
source ${RUCKUS_DIR}/vivado_hls_env_var.tcl
source ${RUCKUS_DIR}/vivado_hls_proc.tcl 
