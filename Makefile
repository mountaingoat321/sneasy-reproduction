CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
PYTHON ?= python3
export MPLCONFIGDIR := $(CURDIR)/build/matplotlib
export OPENBLAS_NUM_THREADS := 1
export OMP_NUM_THREADS := 1
export TEXMFVAR := $(CURDIR)/build/texmf-var
export TEXMFCONFIG := $(CURDIR)/build/texmf-config
export TEXMFCACHE := $(CURDIR)/build/texmf-cache
export TEXMFHOME := $(CURDIR)/build/texmf-home
export VARTEXFONTS := $(CURDIR)/build/texmf-fonts

.PHONY: all test clean report audit
all: test results/ensemble.csv results/reference.csv results/convergence.csv results/forcing_0.csv figures/figure1_nonreproduction.pdf report

build/model: src/main.cpp src/models.cpp src/models.hpp
	mkdir -p build results figures
	$(CXX) $(CXXFLAGS) src/main.cpp src/models.cpp -o $@

build/verify: tests/verify.cpp src/models.cpp src/models.hpp
	mkdir -p build results
	$(CXX) $(CXXFLAGS) tests/verify.cpp src/models.cpp -o $@

data/ocean_layers.csv: scripts/prepare.py
	$(PYTHON) scripts/prepare.py

test: build/verify data/ocean_layers.csv
	./build/verify

results/reference.csv: build/model data/ocean_layers.csv data/co2_annual.csv
	./build/model reference

results/ensemble.csv: build/model data/ocean_layers.csv data/co2_annual.csv
	./build/model ensemble 4096 $@

results/convergence.csv: build/model data/ocean_layers.csv scripts/convergence.py
	$(PYTHON) scripts/convergence.py

results/forcing_0.csv: build/model data/ocean_layers.csv
	./build/model sensitivity

figures/figure1_nonreproduction.pdf: results/ensemble.csv results/reference.csv results/convergence.csv results/forcing_0.csv scripts/analyze.py
	$(PYTHON) scripts/analyze.py

report: figures/figure1_nonreproduction.pdf reports/report.tex
	mkdir -p $(TEXMFVAR) $(TEXMFCONFIG) $(TEXMFCACHE) $(TEXMFHOME) $(VARTEXFONTS)
	pdflatex -interaction=nonstopmode -halt-on-error -output-directory=reports reports/report.tex > build/latex.log
	pdflatex -interaction=nonstopmode -halt-on-error -output-directory=reports reports/report.tex >> build/latex.log

audit:
	$(PYTHON) scripts/audit.py

clean:
	rm -f build/model build/verify results/*.csv results/*.json figures/*.pdf figures/*.png reports/report.pdf reports/generated.tex data/ocean_layers.csv
