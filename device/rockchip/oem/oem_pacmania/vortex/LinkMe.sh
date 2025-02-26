for game in DigDug DigDugII Galaga Galaga88 Galaxian \
	Mappy PacandPal PacLand PacMan PacMania \
	PacManPlus RollingThunder Rompers SuperPacMan ; do
	ln -s /tmp/$game.cfg .
	ln -s /tmp/$game.sav .
done
