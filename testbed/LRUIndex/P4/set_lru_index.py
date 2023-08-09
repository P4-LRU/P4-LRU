import time
import sys

################################################################################

table = bfrt.lru_index.pipe1.Ingress1.set_port_table_0
table.clear()
table.add_with_set_port_action(128, 44)
table.add_with_set_port_action(144, 36)
# table.add_with_set_port_action(128, 144)
# table.add_with_set_port_action(144, 128)

table = bfrt.lru_index.pipe1.Ingress1.set_port_table_1
table.clear()
table.add_with_set_port_action(128, 144)
table.add_with_set_port_action(144, 128)

table = bfrt.lru_index.pipe1.Ingress1.key_1
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.key_2
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.key_3
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.reg_dfa
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.addr_1_1
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.addr_1_2
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.addr_1_3
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.addr_2_1
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.addr_2_2
table.clear()

table = bfrt.lru_index.pipe1.Ingress1.addr_2_3
table.clear()

################################################################################

table = bfrt.lru_index.pipe2.Ingress2.set_port_table
table.clear()
table.add_with_set_port_action(44, 144)
table.add_with_set_port_action(36, 128)

table = bfrt.lru_index.pipe2.Ingress2.key_1
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.key_2
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.key_3
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.reg_dfa
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.addr_1_1
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.addr_1_2
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.addr_1_3
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.addr_2_1
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.addr_2_2
table.clear()

table = bfrt.lru_index.pipe2.Ingress2.addr_2_3
table.clear()
