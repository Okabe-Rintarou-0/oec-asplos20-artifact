#!/bin/bash
#
# generate_f6_plots.sh
# A bash script that generates the downlink deficit plots
#
# Usage: ./generate_f6_plots.sh
# Arguments: None; assumes execution from the top-level scripts directory.
# Outputs: Downlink deficit plot files
#
# Copyright 2019 Bradley Denby
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at <http://www.apache.org/licenses/LICENSE-2.0>.
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

source p3-env/bin/activate
mkdir -p ../plots/compose
python3 plot_f6a.py ../artifacts/ ../plots/
python3 plot_f6b.py ../artifacts/ ../plots/
python3 plot_f6c.py ../artifacts/ ../plots/
python3 plot_f6d.py ../artifacts/ ../plots/compose/
deactivate
