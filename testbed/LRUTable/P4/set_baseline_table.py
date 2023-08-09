import time
import sys

################################################################################

table = bfrt.baseline_table.pipe1.Ingress1.send_table
table.clear()
table.add_with_send(128, 144)
table.add_with_send(144, 128)

table = bfrt.baseline_table.pipe1.Ingress1.key_1
table.clear()

table = bfrt.baseline_table.pipe1.Ingress1.key_2
table.clear()

table = bfrt.baseline_table.pipe1.Ingress1.key_3
table.clear()

table = bfrt.baseline_table.pipe1.Ingress1.value_1
table.clear()

table = bfrt.baseline_table.pipe1.Ingress1.value_2
table.clear()

table = bfrt.baseline_table.pipe1.Ingress1.value_3
table.clear()

################################################################################
