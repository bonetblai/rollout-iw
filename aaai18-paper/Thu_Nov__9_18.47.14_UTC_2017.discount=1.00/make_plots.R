source('make_data.R')
library(ggplot2)
library(grid) # contains unit()
options(digits = 4)


make_plot <- function(d, features, frameskip, mod, planner, time.budget, filename1, filename2, omit.list.for.bars = NULL, my.title = '') {
    w <- d[d$features == features & d$frameskip == frameskip & d$mod == mod & d$planner == planner, c('rom', 'features', 'mod', 'time.budget', 'score.avg', 'human', 'dqn', 'blob', 'score.rel.human', 'score.rel.dqn')]
    f.human <- w[w$time.budget == time.budget & abs(w$score.rel.human) != Inf & abs(w$score.rel.dqn) != Inf & !w$rom %in% omit.list.for.bars, c('rom', 'score.avg', 'human', 'dqn', 'score.rel.human', 'score.rel.dqn', 'score.rel.human')]
    f.dqn <- w[w$time.budget == time.budget & abs(w$score.rel.human) != Inf & abs(w$score.rel.dqn) != Inf & !w$rom %in% omit.list.for.bars, c('rom', 'score.avg', 'human', 'dqn', 'score.rel.human', 'score.rel.dqn', 'score.rel.dqn')]
    colnames(f.human) = c('rom', 'score', 'human', 'dqn', 'score.rel.human', 'score.rel.dqn', 'value')
    colnames(f.dqn) = c('rom', 'score', 'human', 'dqn', 'score.rel.human', 'score.rel.dqn', 'value')
    f.human = transform(f.human, rom = reorder(rom, score.rel.human))
    f.dqn = transform(f.dqn, rom = reorder(rom, score.rel.human))

    p = ggplot(w[w$time.budget != 2 & w$time.budget != 16,], aes(x = human, y = score.avg)) + geom_point(size = 1)
    p.human = ggplot(w[w$time.budget == time.budget & w$score.rel.human > 1 & w$score.avg > 0,], aes(x = reorder(rom, score.rel.human), y = score.rel.human)) + geom_bar(stat = 'identity')
    p.dqn = ggplot(w[w$time.budget == time.budget & w$score.rel.human > 1 & w$score.avg > 0,], aes(x = reorder(rom, score.rel.human), y = score.rel.dqn)) + geom_bar(stat = 'identity')
    p.bars = ggplot(NULL, aes(rom, value)) +
        geom_bar(aes(fill = 'Human'), data = f.human, alpha = 0.75, stat = 'identity') +
        geom_bar(aes(fill = 'DQN'), data = f.dqn, alpha = 0.75, stat = 'identity')

    ###legend_theme = theme(legend.text = element_text(size = 6), legend.direction = 'horizontal', legend.key.size = unit(.25, 'cm'), legend.justification = c('center', 'center'), legend.position = c(0.230, .976), legend.background = element_rect(colour = 'lightgray')) 
    legend_theme = theme(legend.text = element_text(size = 6), legend.direction = 'horizontal', legend.key.size = unit(.25, 'cm'), legend.position = 'top', legend.margin = margin(0, 0, -10,0))
    ###legend.justification = c('left', 'top'), legend.position = c(0, 1), legend.spacing = unit(1, 'cm')))# + labs(x = '', fill = 'Rel. Score'))

    ###plot_theme = theme(plot.margin = unit(c(0, 1, 0, 0), 'mm'))
    plot_theme = theme(plot.margin = unit(c(1, 1, 1, 1), 'mm'))
    pdf(filename1, width = 4, height = 3.25)
    print(p + plot_theme + theme(text = element_text(size = 12)) + facet_wrap(~ time.budget) + geom_abline() + ylab('Rollout score') + xlab('Human score')) # + labs(title = 'Rollout-IW(1) under Different Time Budgets'))
    dev.off()

    plot_theme = theme(plot.margin = unit(c(1, 1, -3, -3.5), 'mm'), panel.grid.major.x = element_line(colour = 'gray', size = .25))
    pdf(filename2, width = 3, height = 3.5)
    if( my.title == '' ) {
      print(p.bars + legend_theme + plot_theme + theme(text = element_text(size = 8)) + coord_flip() + ylab('') + xlab('') + labs(x = '', fill = '') + scale_y_continuous(breaks = c(-30, -20, -10, 0, 10, 20, 30)))
    } else {
      print(p.bars + legend_theme + plot_theme + theme(text = element_text(size = 8)) + coord_flip() + ylab('') + xlab('') + labs(x = '', fill = '', title = my.title) + theme(plot.title = element_text(size = 8)))
    }
    dev.off()

    return(w)
}


# plots
dummy = make_plot(x, 3, 15, '011', 'rollout', 0.5, '011-15-halfsec.pdf', '011-15-halfsec-bars.pdf', c('private eye')) #, my.title = 'Rollout-IW(1) vs. Human vs. DQN')
dummy = make_plot(x, 3, 15, '010', 'rollout', 0.5, '010-15-halfsec.pdf', '010-15-halfsec-bars.pdf', c('private eye'))
dummy = make_plot(x, 3, 15, '110', 'rollout', 0.5, '110-15-halfsec.pdf', '110-15-halfsec-bars.pdf')# c('private eye'))
#make_plot(x, 3, 15, '111', 'rollout', 0.5, '111-15-halfsec.pdf', '111-15-halfsec-bars.pdf', c('private eye'))

#make_plot(x, 3, 15, '010', 'rollout', 1.0, '010-15-onesec.pdf', '010-15-onesec-bars.pdf', c('private eye'))
#make_plot(x, 3, 15, '011', 'rollout', 1.0, '011-15-onesec.pdf', '011-15-onesec-bars.pdf', c('private eye')) #, my.title = 'Rollout-IW(1) vs. Human vs. DQN')
#make_plot(x, 3, 15, '110', 'rollout', 1.0, '110-15-onesec.pdf', '110-15-onesec-bars.pdf', c('private eye')) #, my.title = 'Rollout-IW(1) vs. Human vs. DQN')
#make_plot(x, 3, 15, '111', 'rollout', 1.0, '111-15-onesec.pdf', '111-15-onesec-bars.pdf', c('private eye'))

