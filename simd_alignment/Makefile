CMAKE := cmake

.PHONY: all clean

all: release

release: requirements
	$(CMAKE) -B build -S . -DCMAKE_BUILD_TYPE=Release
	$(MAKE) -C build

requirements:
	@if [ -z "$(XSIMD_HOME)" ]; then \
		echo "\033[31mError: XSIMD_HOME is not set\033[0m"; \
		exit 1; \
	fi

clean:
	rm -rf build/ *.a
