library(plyr)
options(width=350, digits=2)


# utility function to trim decimal places
specify_decimal <- function(x, k) trimws(format(round(x, k), nsmall=k))


# read .csv tables
d10 = read.csv('data10.csv', header=T)
#d11 = read.csv('data11.csv', header=T)
d11n = read.csv('data11n.csv', header=T)


# patch rom names
levels(d10$rom) = sub('jamesbond', 'james bond', gsub('_', ' ', levels(d10$rom)))
#levels(d11$rom) = sub('jamesbond', 'james bond', gsub('_', ' ', levels(d11$rom)))
levels(d11n$rom) = sub('jamesbond', 'james bond', gsub('_', ' ', levels(d11n$rom)))


# add columns:
#  caching: 0=none, 1=partial, 2=full
#  mod for mode: 1st bit = subtables, 2nd bit = caching, 3rd bit = random actions
#  perc.random.decisions: percentage of random decisions
#  avg.expanded: average expanded nodes per decision made
#  avg.simulator.calls: average number of simulator calls per decision made

d10$caching = rep(1, dim(d10)[1])
d10$mod = paste(d10$novelty.subtables, d10$caching, d10$random.actions, sep='')
d10$perc.random.decisions = d10$random.decisions / d10$decisions
d10$avg.expanded = d10$sum.expanded / d10$decisions
d10$avg.simulator.calls = d10$simulator.calls / d10$decisions

#d11$caching = rep(1, dim(d11)[1])
#d11$mod = paste(d11$novelty.subtables, d11$caching, d11$random.actions, sep='')
#d11$perc.random.decisions = d11$random.decisions / d11$decisions
#d11$avg.expanded = d11$sum.expanded / d11$decisions
#d11$avg.simulator.calls = d11$simulator.calls / d11$decisions

d11n$caching = rep(1, dim(d11n)[1])
d11n$mod = paste(d11n$novelty.subtables, d11n$caching, d11n$random.actions, sep='')
d11n$perc.random.decisions = d11n$random.decisions / d11n$decisions
d11n$avg.expanded = d11n$sum.expanded / d11n$decisions
d11n$avg.simulator.calls = d11n$simulator.calls / d11n$decisions


# create data frame d by appending all data row wise
#d = rbind(d00, d01, d10, d11, d20, d21)
d = rbind(d10, d11n) # replace d11 by d11n
table4 = read.csv('../table4.csv', header=T)
table5 = read.csv('../table5.csv', header=T)
table.iw = read.csv('../table_iw.csv', header=T)
table.ram = read.csv('../table_ram.csv', header=T)


# merge tables
aux1 = merge(d, table4, by = 'rom')
aux2 = merge(aux1, table5, by = 'rom')
#aux1 = merge(aux2, table.iw, by = 'rom')
#mr = merge(aux1, table.ram, by = 'rom')
mr = aux2

aux1 = merge(table4, table5, by = 'rom')
#aux2 = merge(aux1, table.iw, by = 'rom')
#table.external = merge(aux2, table.ram, by = 'rom')
table.external = merge(aux1, table.ram, by = 'rom')


# create comparison of baselines
#baseline.comparison = matrix(rep(0, 16), nrow=4)
#rownames(baseline.comparison) = c('ram', 'dqn', 'blob', 'human')
#colnames(baseline.comparison) = c('ram', 'dqn', 'blob', 'human')
#baseline.comparison['ram', 'dqn'] = mean(table.external$ram.score > table.external$dqn.avg)
#baseline.comparison['ram', 'blob'] = mean(table.external$ram.score > table.external$blob.full.avg)
#baseline.comparison['ram', 'human'] = mean(table.external$ram.score > table.external$human.score)
#baseline.comparison['dqn', 'ram'] = mean(table.external$dqn.avg > table.external$ram.score)
#baseline.comparison['dqn', 'blob'] = mean(table.external$dqn.avg > table.external$blob.full.avg)
#baseline.comparison['dqn', 'human'] = mean(table.external$dqn.avg > table.external$human.score)
#baseline.comparison['blob', 'ram'] = mean(table.external$blob.full.avg > table.external$ram.score)
#baseline.comparison['blob', 'dqn'] = mean(table.external$blob.full.avg > table.external$dqn.avg)
#baseline.comparison['blob', 'human'] = mean(table.external$blob.full.avg > table.external$human.score)
#baseline.comparison['human', 'ram'] = mean(table.external$human.score > table.external$ram.score)
#baseline.comparison['human', 'dqn'] = mean(table.external$human.score > table.external$dqn.avg)
#baseline.comparison['human', 'blob'] = mean(table.external$human.score > table.external$blob.full.avg)


relative_score <- function(score, base) if( score == base ) 0 else (if( score >= base ) abs(score-base)/abs(base) else -abs(base-score)/abs(score))

quantile.human <- function(w, mod, planner, budget, alpha) {
  myw = w[w$mod == mod & w$planner == planner & w$time.budget == budget,]
  return(c(sum(myw$score.avg >= alpha * myw$human), dim(myw)[1]))
}
quantile.dqn <- function(w, mod, planner, budget, alpha) {
  myw = w[w$mod == mod & w$planner == planner & w$time.budget == budget,]
  return(c(sum(myw$score.avg >= alpha * myw$dqn), dim(myw)[1]))
}

compare <- function(w, mod1, planner1, mod2, planner2, budget) {
  return(sum(w[w$mod == mod1 & w$planner == planner1 & w$time.budget == budget,]$score.avg >= w[w$mod == mod2 & w$planner == planner2 & w$time.budget == budget,]$score.avg))
}
row.compare <- function(w, mod, planner, budget) {
  r = c(compare(w, mod, planner, '010', planner, budget),
        compare(w, mod, planner, '011', planner, budget),
        compare(w, mod, planner, '110', planner, budget),
        compare(w, mod, planner, '111', planner, budget))
  return(c(r, 100 * r / 49))
}

# summarize data
x = ddply(mr, c('rom', 'frameskip', 'features', 'mod', 'discount', 'time.budget', 'planner'), summarise,
          episodes = length(score),
          score.avg = mean(score),
          score.sd = sd(score),
          frames.avg = mean(frames),
          frames.sd = sd(frames),
          decisions.avg = mean(decisions),
          decisions.sd = sd(decisions),
          time.avg = mean(total.time),
          time.sd = sd(total.time),
          ###avg.score = score / frames,
          ###avg.time = avg.time.frame,
          ###time.dec = mean(avg.time.dec),
          perc.random.decisions = mean(perc.random.decisions),
          avg.expanded = mean(avg.expanded),
          max.sim.calls = mean(max.simulator.calls),
          avg.sim.calls = mean(avg.simulator.calls),
          ###ram = mean(ram.score),
          human = mean(human.score),
          dqn = mean(dqn.avg),
          blob = mean(blob.full.avg),
          #score.rel.human = (2*sign(score.avg > human) - 1) * abs(sum(score.avg) - sum(human)) / abs(sum(human)),
          #dqn.rel.human = (2*sign(dqn > human) - 1) * abs(sum(dqn) - sum(human)) / abs(sum(human)))
          score.rel.human = relative_score(sum(score.avg), sum(human)),
          score.rel.dqn = relative_score(sum(score.avg), sum(dqn))
          ###iw = iw.score,
          ###bfs2 = bfs2.score,
          ###bfrs = bfrs.score,
          ###uct = uct.score)
          ###best = score > human & score > dqn & score > ram,
          ###better = score > human & score > dqn,
          ###worse = score < human | score < dqn
)

z = ddply(x[x$features == 3, ], c('frameskip', 'mod', 'discount', 'time.budget', 'planner'), summarise,
          num.roms = length(score.avg),
          avg.episodes = mean(episodes),
          #acc.avg.score = sum(score.avg),
          better.than.human = mean(score.avg > human),
          better.than.dqn = mean(score.avg > dqn),
          ###better.than.ram = mean(score.avg > ram),
          better.than.blob = mean(score.avg > blob),
          score.rel.human = sum(score.rel.human),
          score.rel.dqn = sum(score.rel.dqn)
)

x.d = ddply(d, c('rom', 'frameskip', 'features', 'mod', 'discount', 'time.budget', 'planner'), summarise,
          episodes = length(score),
          score.avg = mean(score),
          score.sd = sd(score),
          frames.avg = mean(frames),
          frames.sd = sd(frames),
          decisions.avg = mean(decisions),
          decisions.sd = sd(decisions),
          time.avg = mean(total.time),
          time.sd = sd(total.time),
          ###avg.score = score / frames,
          ###avg.time = avg.time.frame,
          ###time.dec = mean(avg.time.dec),
          perc.random.decisions = mean(perc.random.decisions),
          avg.expanded = mean(avg.expanded),
          max.sim.calls = mean(max.simulator.calls),
          avg.sim.calls = mean(avg.simulator.calls)
          ###ram = mean(ram.score),
          ###human = mean(human.score),
          ###dqn = mean(dqn.avg),
          ###blob = mean(blob.full.avg),
          #score.rel.human = (2*sign(score.avg > human) - 1) * abs(sum(score.avg) - sum(human)) / abs(sum(human)),
          #dqn.rel.human = (2*sign(dqn > human) - 1) * abs(sum(dqn) - sum(human)) / abs(sum(human)))
          ###score.rel.human = relative_score(sum(score.avg), sum(human)), #(2*sign(score.avg > human) - 1) * abs(sum(score.avg)) / abs(sum(human)),
          ###score.rel.dqn = relative_score(sum(score.avg), sum(dqn))) #(2*sign(score.avg > dqn) - 1) * abs(sum(score.avg)) / abs(sum(dqn)))
          ###iw = iw.score,
          ###bfs2 = bfs2.score,
          ###bfrs = bfrs.score,
          ###uct = uct.score)
          ###best = score > human & score > dqn & score > ram,
          ###better = score > human & score > dqn,
          ###worse = score < human | score < dqn
)


