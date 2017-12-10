source('make_data.R')

options(digits = 10)

# subtables 011: (Plain) Rollout IW(1)
roll.011.15.5s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '011' & x.d$time.budget == 0.5, c('rom', 'score.avg')]
colnames(roll.011.15.5s) = c('rom', 'roll.011.15.5s')
roll.011.15.1s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '011' & x.d$time.budget == 1, c('rom', 'score.avg')]
colnames(roll.011.15.1s) = c('rom', 'roll.011.15.1s')
roll.011.15.32s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '011' & x.d$time.budget == 32, c('rom', 'score.avg')]
colnames(roll.011.15.32s) = c('rom', 'roll.011.15.32s')

# subtables 010: RA Rollout IW(1)
roll.010.15.5s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '010' & x.d$time.budget == 0.5, c('rom', 'score.avg')]
colnames(roll.010.15.5s) = c('rom', 'roll.010.15.5s')
roll.010.15.1s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '010' & x.d$time.budget == 1, c('rom', 'score.avg')]
colnames(roll.010.15.1s) = c('rom', 'roll.010.15.1s')
roll.010.15.32s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '010' & x.d$time.budget == 32, c('rom', 'score.avg')]
colnames(roll.010.15.32s) = c('rom', 'roll.010.15.32s')

# subtables 111: NS Rollout IW(1)
roll.111.15.5s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '111' & x.d$time.budget == 0.5, c('rom', 'score.avg')]
colnames(roll.111.15.5s) = c('rom', 'roll.111.15.5s')
roll.111.15.1s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '111' & x.d$time.budget == 1, c('rom', 'score.avg')]
colnames(roll.111.15.1s) = c('rom', 'roll.111.15.1s')
roll.111.15.32s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '111' & x.d$time.budget == 32, c('rom', 'score.avg')]
colnames(roll.111.15.32s) = c('rom', 'roll.111.15.32s')

# subtables 110: NS+RA Rollout IW(1)
roll.110.15.5s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '110' & x.d$time.budget == 0.5, c('rom', 'score.avg')]
colnames(roll.110.15.5s) = c('rom', 'roll.110.15.5s')
roll.110.15.1s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '110' & x.d$time.budget == 1, c('rom', 'score.avg')]
colnames(roll.110.15.1s) = c('rom', 'roll.110.15.1s')
roll.110.15.32s = x.d[x.d$planner == 'rollout' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '110' & x.d$time.budget == 32, c('rom', 'score.avg')]
colnames(roll.110.15.32s) = c('rom', 'roll.110.15.32s')

# subtables 011: Plain (BFS) IW(1)
bfs.011.15.5s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '011' & x.d$time.budget == 0.5, c('rom', 'score.avg')]
colnames(bfs.011.15.5s) = c('rom', 'bfs.011.15.5s')
bfs.011.15.1s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '011' & x.d$time.budget == 1, c('rom', 'score.avg')]
colnames(bfs.011.15.1s) = c('rom', 'bfs.011.15.1s')
bfs.011.15.32s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '011' & x.d$time.budget == 32, c('rom', 'score.avg')]
colnames(bfs.011.15.32s) = c('rom', 'bfs.011.15.32s')

# subtables 010: RA (BFS) IW(1)
bfs.010.15.1s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '010' & x.d$time.budget == 1, c('rom', 'score.avg')]
colnames(bfs.010.15.1s) = c('rom', 'bfs.010.15.1s')
bfs.010.15.32s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '010' & x.d$time.budget == 32, c('rom', 'score.avg')]
colnames(bfs.010.15.32s) = c('rom', 'bfs.010.15.32s')

# subtables 111: NS (BFS) IW(1)
bfs.111.15.1s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '111' & x.d$time.budget == 1, c('rom', 'score.avg')]
colnames(bfs.111.15.1s) = c('rom', 'bfs.111.15.1s')
bfs.111.15.32s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '111' & x.d$time.budget == 32, c('rom', 'score.avg')]
colnames(bfs.111.15.32s) = c('rom', 'bfs.111.15.32s')

# subtables 110: NS+RA (BFS) IW(1)
bfs.110.15.1s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '110' & x.d$time.budget == 1, c('rom', 'score.avg')]
colnames(bfs.110.15.1s) = c('rom', 'bfs.110.15.1s')
bfs.110.15.32s = x.d[x.d$planner == 'bfs' & x.d$frameskip == 15 & x.d$features == 3 & x.d$mod == '110' & x.d$time.budget == 32, c('rom', 'score.avg')]
colnames(bfs.110.15.32s) = c('rom', 'bfs.110.15.32s')



# table1a: rom, human, dqn, blob, roll.011.15.1s
table1.base = x[x$planner == 'rollout' & x$features == 3 & x$mod == '011' & x$time.budget == 1, c('rom', 'human', 'dqn', 'blob')]
table1.011.15.1s = join(table1.base, roll.011.15.1s, by = 'rom')
table1.110.15.5s = join(table1.base, roll.110.15.5s, by = 'rom')

# count # times each (pixel) algorithm is >= human
table1.011.as.good.as.human = as.list(rep(0, 1 + ncol(table1.011.15.1s))) # number of cols is increased by 1 below
table1.011.as.good.as.human[1] = '\\# as good as Human'
table1.011.as.good.as.human[2] = NA
table1.011.as.good.as.human[3] = NA
for( i in 3:ncol(table1.011.15.1s) )
    table1.011.as.good.as.human[i] = sum(table1.011.15.1s[, i] >= table1.011.15.1s$human, na.rm = TRUE)

table1.011.as.good.as.75human = as.list(rep(0, 1 + ncol(table1.011.15.1s))) # number of cols is increased by 1 below
table1.011.as.good.as.75human[1] = '\\# as good as 75\\% of Human'
table1.011.as.good.as.75human[2] = NA
table1.011.as.good.as.75human[3] = NA
for( i in 3:ncol(table1.011.15.1s) )
    table1.011.as.good.as.75human[i] = sum(table1.011.15.1s[, i] >= 0.75 * table1.011.15.1s$human, na.rm = TRUE)

table1.110.as.good.as.human = as.list(rep(0, 1 + ncol(table1.110.15.5s))) # number of cols is increased by 1 below
table1.110.as.good.as.human[1] = '\\# as good as Human'
table1.110.as.good.as.human[2] = NA
table1.110.as.good.as.human[3] = NA
for( i in 3:ncol(table1.110.15.5s) )
    table1.110.as.good.as.human[i] = sum(table1.110.15.5s[, i] >= table1.110.15.5s$human, na.rm = TRUE)

table1.110.as.good.as.75human = as.list(rep(0, 1 + ncol(table1.110.15.5s))) # number of cols is increased by 1 below
table1.110.as.good.as.75human[1] = '\\# as good as 75\\% of Human'
table1.110.as.good.as.75human[2] = NA
table1.110.as.good.as.75human[3] = NA
for( i in 3:ncol(table1.110.15.5s) )
    table1.110.as.good.as.75human[i] = sum(table1.110.15.5s[, i] >= 0.75 * table1.110.15.5s$human, na.rm = TRUE)

# compute # best in game
table1.011.15.1s$best.score = apply(table1.011.15.1s[, 2:ncol(table1.011.15.1s)], 1, max)
table1.011.best.in.game = as.list(rep(0, ncol(table1.011.15.1s)))
table1.011.best.in.game[1] = '\\# best in game'
for( i in 2:ncol(table1.011.15.1s) )
    table1.011.best.in.game[i] = sum(table1.011.15.1s[, i] == table1.011.15.1s$best.score, na.rm = TRUE)

table1.110.15.5s$best.score = apply(table1.110.15.5s[, 2:ncol(table1.110.15.5s)], 1, max)
table1.110.best.in.game = as.list(rep(0, ncol(table1.110.15.5s)))
table1.110.best.in.game[1] = '\\# best in game'
for( i in 2:ncol(table1.110.15.5s) )
    table1.110.best.in.game[i] = sum(table1.110.15.5s[, i] == table1.110.15.5s$best.score, na.rm = TRUE)

# attach new rows to table1
levels(table1.011.15.1s$rom) = c(levels(table1.011.15.1s$rom), '\\# as good as Human', '\\# as good as 75\\% of Human', '\\# best in game')
table1.011.15.1s = rbind(table1.011.15.1s, table1.011.as.good.as.human, table1.011.as.good.as.75human, table1.011.best.in.game)
levels(table1.110.15.5s$rom) = c(levels(table1.110.15.5s$rom), '\\# as good as Human', '\\# as good as 75\\% of Human', '\\# best in game')
table1.110.15.5s = rbind(table1.110.15.5s, table1.110.as.good.as.human, table1.110.as.good.as.75human, table1.110.best.in.game)

# print table
sink('table1-011-15-onesec.txt')
format(table1.011.15.1s, digits = 4, nsmall = 1, big.mark = ',')
sink()

sink('table1-110-15-halfsec.txt')
format(table1.110.15.5s, digits = 4, nsmall = 1, big.mark = ',')
sink()



# table2a and table2b
aux1 = join(table5[, c('rom', 'human.score')], table.iw[, c('rom', 'iw.score')], by = 'rom', type = 'full')
colnames(aux1) = c('rom', 'human', 'ram')
aux2 = join(aux1, bfs.011.15.5s[, c('rom', 'bfs.011.15.5s')], by = 'rom', type = 'full')
aux1 = join(aux2, bfs.011.15.1s[, c('rom', 'bfs.011.15.1s')], by = 'rom', type = 'full')
aux2 = join(aux1, bfs.011.15.32s[, c('rom', 'bfs.011.15.32s')], by = 'rom', type = 'full')
aux1 = join(aux2, roll.011.15.5s[, c('rom', 'roll.011.15.5s')], by = 'rom', type = 'full')
aux2 = join(aux1, roll.011.15.1s[, c('rom', 'roll.011.15.1s')], by = 'rom', type = 'full')
aux1 = join(aux2, roll.011.15.32s[, c('rom', 'roll.011.15.32s')], by = 'rom', type = 'full')
aux2 = join(aux1, roll.010.15.5s[, c('rom', 'roll.010.15.5s')], by = 'rom', type = 'full')
aux1 = join(aux2, roll.010.15.1s[, c('rom', 'roll.010.15.1s')], by = 'rom', type = 'full')
aux2 = join(aux1, roll.010.15.32s[, c('rom', 'roll.010.15.32s')], by = 'rom', type = 'full')
aux1 = join(aux2, roll.110.15.5s[, c('rom', 'roll.110.15.5s')], by = 'rom', type = 'full')
aux2 = join(aux1, roll.110.15.1s[, c('rom', 'roll.110.15.1s')], by = 'rom', type = 'full')
aux1 = join(aux2, roll.110.15.32s[, c('rom', 'roll.110.15.32s')], by = 'rom', type = 'full')
# table 2a: human, IW(1) RAM, IW(1), Rollout IW(1), RA Rollout IW(1) and RA+NS Rollout IW(1) for .5s, 1s and 32s
table2a = aux1[aux1$rom != 'carnival' & aux1$rom != 'journey escape' & aux1$rom != 'pooyan', ]
# table 2b: human, IW(1) RAM, IW(1), Rollout IW(1), RA Rollout IW(1) and RA+NS Rollout IW(1) for .5s and 32s
table2b = table2a[, c('rom', 'human', 'ram', 'bfs.011.15.5s', 'bfs.011.15.32s', 'roll.011.15.5s', 'roll.011.15.32s', 'roll.010.15.5s', 'roll.010.15.32s', 'roll.110.15.5s', 'roll.110.15.32s')]

# count # times each (pixel) algorithm is >= human
table2a.as.good.as.human = as.list(rep(0, 1 + ncol(table2a))) # number of cols is increased by 1 below
table2a.as.good.as.human[1] = '\\# as good as Human'
table2a.as.good.as.human[2] = NA
table2a.as.good.as.human[3] = NA
for( i in 4:ncol(table2a) )
    table2a.as.good.as.human[i] = sum(table2a[, i] >= table2a$human, na.rm = TRUE)

table2a.as.good.as.75human = as.list(rep(0, 1 + ncol(table2a))) # number of cols is increased by 1 below
table2a.as.good.as.75human[1] = '\\# as good as 75\\% of Human'
table2a.as.good.as.75human[2] = NA
table2a.as.good.as.75human[3] = NA
for( i in 4:ncol(table2a) )
    table2a.as.good.as.75human[i] = sum(table2a[, i] >= 0.75 * table2a$human, na.rm = TRUE)

table2b.as.good.as.human = as.list(rep(0, 1 + ncol(table2b))) # number of cols is increased by 1 below
table2b.as.good.as.human[1] = '\\# as good as Human'
table2b.as.good.as.human[2] = NA
table2b.as.good.as.human[3] = NA
for( i in 4:ncol(table2b) )
    table2b.as.good.as.human[i] = sum(table2b[, i] >= table2b$human, na.rm = TRUE)

table2b.as.good.as.75human = as.list(rep(0, 1 + ncol(table2b))) # number of cols is increased by 1 below
table2b.as.good.as.75human[1] = '\\# as good as 75\\% of Human'
table2b.as.good.as.75human[2] = NA
table2b.as.good.as.75human[3] = NA
for( i in 4:ncol(table2b) )
    table2b.as.good.as.75human[i] = sum(table2b[, i] >= 0.75 * table2b$human, na.rm = TRUE)

# compute # best in game
table2a[is.na(table2a)] = -1e6
table2a$best.score = apply(table2a[, 2:ncol(table2a)], 1, max)

table2a.best.in.game = as.list(rep(0, ncol(table2a)))
table2a.best.in.game[1] = '\\# best in game'
for( i in 2:ncol(table2a) )
    table2a.best.in.game[i] = sum(table2a[, i] == table2a$best.score, na.rm = TRUE)

table2b[is.na(table2b)] = -1e6
table2b$best.score = apply(table2b[, 2:ncol(table2b)], 1, max)

table2b.best.in.game = as.list(rep(0, ncol(table2b)))
table2b.best.in.game[1] = '\\# best in game'
for( i in 2:ncol(table2b) )
    table2b.best.in.game[i] = sum(table2b[, i] == table2b$best.score, na.rm = TRUE)

# restore NAs
table2a[table2a == -1e6] = NA
table2b[table2b == -1e6] = NA

# attach new rows to table2a and table2b
levels(table2a$rom) = c(levels(table2a$rom), '\\# as good as Human', '\\# as good as 75\\% of Human', '\\# best in game')
table2a = rbind(table2a, table2a.as.good.as.human, table2a.as.good.as.75human, table2a.best.in.game)
levels(table2b$rom) = c(levels(table2b$rom), '\\# as good as Human', '\\# as good as 75\\% of Human', '\\# best in game')
table2b = rbind(table2b, table2b.as.good.as.human, table2b.as.good.as.75human, table2b.best.in.game)

# generate tables
sink('table2a.txt')
format(table2a, digits = 4, nsmall = 1, big.mark = ',')
sink()

sink('table2b.txt')
format(table2b, digits = 4, nsmall = 1, big.mark = ',')
sink()

