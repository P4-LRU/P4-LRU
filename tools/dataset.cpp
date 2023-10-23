#include <bits/stdc++.h>

using namespace std;

#pragma pack(push)
#pragma pack(1)
struct Trace {
  uint8_t bytes[15];
  double tstamp;
};
#pragma pack(pop)

bool operator<(const Trace &a, const Trace &b) { return a.tstamp < b.tstamp; }

void readTrace(int n, vector<Trace> &traces, double period) {
  char filename[256];
  sprintf(filename, "./CAIDA_2018/%02d.dat", n);

  cout << filename << endl;

  ifstream file(filename, ios::binary);

  Trace trace;

  double init_tstamp = 0;
  int nb_traces = 0;
  while (file.read((char *)&trace, 23)) {
    if (init_tstamp == 0) {
      init_tstamp = floor(trace.tstamp);
    }

    trace.tstamp = (trace.tstamp - init_tstamp) / period;

    if (trace.tstamp > 1.0) {
      break;
    } else {
      traces.push_back(trace);
      nb_traces += 1;
    }
  }
  cout << nb_traces << endl;

  file.close();
}

void writeTrace(int nb, vector<Trace> &traces) {
  char filename[256];
  sprintf(filename, "./CAIDA_BYTE_TSTAMP_2018/normalized_%03d.dat", nb);

  ofstream file(filename, ios::binary);

  for (int i = 0; i < traces.size(); i++) {
    file.write((const char *)traces[i].bytes, 23);
  }

  file.close();
}

vector<Trace> traces;

int main(int argc, char **argv) {
  assert(argc == 2);

  int nb_datasets = atoi(argv[1]);

  double period = 60.0 / nb_datasets;

  for (int i = 0; i < nb_datasets; i++) {
    readTrace(i, traces, period);
  }

  // for (int i = 0; i < traces.size(); i++) {
  //   cout << traces[i].tstamp << endl;
  // }

  sort(traces.begin(), traces.end());

  writeTrace(nb_datasets, traces);
}
