BIT_FILE = project/project.runs/impl_1/GROWTH_FY2016_FPGA.bit
IMAGE_OUTPUT_DIR = bitstream

all: $(BIT_FILE)
	@install -d $(IMAGE_OUTPUT_DIR)
	@cp $(BIT_FILE) $(IMAGE_OUTPUT_DIR)
	@echo "Bitstream file saved to $(IMAGE_OUTPUT_DIR)"

$(BIT_FILE): tcl/create_project_then_compile.tcl
	echo "If vivado is not found, download from https://www.xilinx.com/support/download.html and install."
	vivado -mode batch -source tcl/create_project_then_compile.tcl

clean:
	rm -rf project $(IMAGE_OUTPUT_DIR)