all: build-sender build-receiver

VERSION := $(shell yt --version 2> /dev/null)
HEX_FILE ?= build/bbc-microbit-classic-gcc/source/microbit-samples-combined.hex
ROLE_HEADER := source/build_role.h
ARTIFACT_DIR := artifacts
SENDER_HEX := $(ARTIFACT_DIR)/microbit-sender-combined.hex
RECEIVER_HEX := $(ARTIFACT_DIR)/microbit-receiver-combined.hex
MICROBIT_MOUNT ?= /host/Volumes/MICROBIT

.PHONY: all check build build-sender build-receiver flash flash-sender flash-receiver install clean set-role-sender set-role-receiver ensure-artifact-dir

check:
ifeq ($(VERSION),)
	@echo you should use before trying anything: source /sync/Module_Dev_app_mobile/yotta/bin/activate
	@false
endif

ensure-artifact-dir:
	@mkdir -p "$(ARTIFACT_DIR)"

set-role-sender:
	@printf '%s\n' '#ifndef BUILD_ROLE_H' '#define BUILD_ROLE_H' '' '#define BUILD_ROLE_SENDER 1' '#define BUILD_ROLE_RECEIVER 2' '' '#define BUILD_ROLE BUILD_ROLE_SENDER' '' '#endif' > "$(ROLE_HEADER)"

set-role-receiver:
	@printf '%s\n' '#ifndef BUILD_ROLE_H' '#define BUILD_ROLE_H' '' '#define BUILD_ROLE_SENDER 1' '#define BUILD_ROLE_RECEIVER 2' '' '#define BUILD_ROLE BUILD_ROLE_RECEIVER' '' '#endif' > "$(ROLE_HEADER)"

build: build-sender

build-sender: check set-role-sender ensure-artifact-dir
	@yt clean
	@yt build
	@cp "$(HEX_FILE)" "$(SENDER_HEX)"
	@echo "Build sender ok -> $(SENDER_HEX)"

build-receiver: check set-role-receiver ensure-artifact-dir
	@yt clean
	@yt build
	@cp "$(HEX_FILE)" "$(RECEIVER_HEX)"
	@echo "Build receiver ok -> $(RECEIVER_HEX)"

flash: flash-sender

flash-sender: build-sender
	@if [ ! -f "$(SENDER_HEX)" ]; then echo "HEX sender introuvable: $(SENDER_HEX)"; exit 1; fi
	@if [ ! -d "$(MICROBIT_MOUNT)" ]; then echo "Volume MICROBIT non monte: $(MICROBIT_MOUNT)"; exit 1; fi
	@cp "$(SENDER_HEX)" "$(MICROBIT_MOUNT)/"
	@echo "Flash sender ok -> $(MICROBIT_MOUNT)"

flash-receiver: build-receiver
	@if [ ! -f "$(RECEIVER_HEX)" ]; then echo "HEX receiver introuvable: $(RECEIVER_HEX)"; exit 1; fi
	@if [ ! -d "$(MICROBIT_MOUNT)" ]; then echo "Volume MICROBIT non monte: $(MICROBIT_MOUNT)"; exit 1; fi
	@cp "$(RECEIVER_HEX)" "$(MICROBIT_MOUNT)/"
	@echo "Flash receiver ok -> $(MICROBIT_MOUNT)"

install: flash
	@echo "Install done"

clean: check
	@yt clean
	@echo "Cleaning done"
